#ifndef DEC_USER_INFO
#define DEC_USER_INFO
#include "popover.h"

#include "component.h"
#include "../discord/structs.h"
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

  void initFrom(DiscordRichUser user, DiscordPresence presence);
  DiscordRichUser user;
  DiscordPresence presence;
  Image* user_image = nullptr;
  TextWithState username_text;
  TextBox t_username;
  TextWithState bio_text;
  TextBox t_bio;
  TextBox t_state;
  TextWithState state_text;
};
#endif