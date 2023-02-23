#ifndef DEC_USER_INFO
#define DEC_USER_INFO
#include <vector>
#include "popover.h"
#include "search_list.h"
#include "component.h"
#include "../rendering/text.h"
#include "../rendering/text_box.h"
#include "../rendering/box.h"
#include "../rendering/image.h"

class UserInfo : public Popover, public Component {
 public:
  UserInfo();
  bool canFocus() override;
  void onFocus(bool focus) override;
  void render(float x, float y, float w, float h) override;
  void render(float width, float height) override;

  void initFrom(WhoIsEntry);
  Image* user_image = nullptr;
  WhoIsEntry entry;
  TextWithState username_text;
  TextBox t_username;
  TextWithState server_text;
  TextBox t_server;
  TextWithState meta_text;
  TextBox t_meta;
  TextWithState bio_text;
  TextBox t_bio;
  TextBox t_state;
  TextWithState state_text;
  SearchList channel_list;
  std::vector<SearchItem> channel_items;
};
#endif