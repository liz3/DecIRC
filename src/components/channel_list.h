#ifndef DEC_CHANNEL_LIST_H
#define DEC_CHANNEL_LIST_H

#include <string>
#include <vector>
#include "component.h"
#include "popover.h"
#include "../rendering/text_box.h"
#include "../irc/irc_client.h"
#include "search_list.h"
#include "../rendering/box.h"

enum QueryPopulateType {
    List,
    Names,
    QuickSearch
};

class ChannelList : public Popover, public Component {
 public:
  ChannelList();
  bool canFocus() override;
  void onFocus(bool focus) override;
  void render(float x, float y, float w, float h) override;
  void render(float width, float height) override;
  void initFrom(IrcClient* client, QueryPopulateType what);
  void initQuickSearch(std::vector<IrcChannel*> channels);
  SearchList search_list;
  TextWithState query_text;
  TextBox query_box;

 private:
  std::vector<SearchItem> items;
  IrcClient* network = nullptr;
  QueryPopulateType mode;
};
#endif