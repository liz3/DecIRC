#ifndef DEC_USERLIST_H
#define DEC_USERLIST_H
#include "popover.h"

#include "component.h"
#include "search_list.h"
#include "../rendering/text.h"
#include "../rendering/text_box.h"
#include "../rendering/box.h"
#include "../rendering/image.h"

class UserList : public Popover, public Component {
 public:
  UserList();
  bool canFocus() override;
  void onFocus(bool focus) override;
  void render(float x, float y, float w, float h) override;
  void render(float width, float height) override;

  void initFrom(IrcChannel& channel);
  SearchList& getList();

 private:
  SearchList user_list;
  std::vector<SearchItem> items;
};
#endif