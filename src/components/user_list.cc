#include "user_list.h"

UserList::UserList() {
  user_list.setItems(&items);
  user_list.title.setData("Users");
}
bool UserList::canFocus() {
  return true;
}
void UserList::onFocus(bool focus) {
  if (focus)
    AppState::gState->setTextReceiver(&user_list, &user_list);
}

void UserList::render(float window_width, float window_height) {
  auto w = 360 > window_width ? window_width - 45 : 360;
  auto h = 800 > window_height ? window_height - 45 : 800;
  FontAtlas* atlas = &AppState::gState->atlas;
  render(window_width - w - 20, 10, w, h);
  user_list.render(window_width - w - 20, 10 + atlas->effective_atlas_height, w,
                   h - 45);
}
SearchList& UserList::getList() {
  return user_list;
}
void UserList::initFrom(IrcChannel& channel) {
  items.clear();
  for (const auto& r : channel.users) {
    SearchItem item;
    item.name = r;
    item.user_data = nullptr;
    items.push_back(item);
  }
  user_list.setItems(&items);
  user_list.title.setData("Users (" + std::to_string(channel.users.size()) +
                          ")");
}
void UserList::render(float x, float y, float w, float h) {
  auto abs = AppState::gState->getPositionAbsolute(x, y, 0, 0);
  x = abs.x;
  y = abs.y;

  Box::render(x, y, w, -h, vec4f(0.1, 0.1, 0.15, 1));
}