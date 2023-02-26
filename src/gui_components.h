#ifndef DEC_GUI_COMPONENTS
#define DEC_GUI_COMPONENTS
class AppState;

#include "./components/chat_input.h"
#include "./components/message_list.h"
#include "./components/search_list.h"
#include "./components/text_field.h"
#include "./components/user_info.h"
#include "./components/user_list.h"
#include "./components/channel_list.h"
#include "./components/image_overlay.h"

#include "./rendering/image.h"
#include <mutex>
#include <thread>

using GuiThreadTask = std::function<void()>*;
class GuiComponents {
 public:
  std::mutex mtx;
  uint8_t root_list_display = 0;
  AppState* state;
  ChatInput chat_input;
  SearchList dm_list;
  SearchList channel_list;
  SearchList network_list;
  TextBox header_comp;
  TextWithState header_text;
  TextBox status_comp;
  TextWithState status_text;
  std::vector<GuiThreadTask> tasks;
  MessageList message_list;
  std::thread::id this_id = std::this_thread::get_id();
  bool locked = false;
  Popover* active_popover = nullptr;

  UserInfo userInfo;
  ChannelList channels_popover;
  ImageOverlay imageOverlay;
  UserList user_list;
  Image testImage;
  GuiComponents(AppState* _state);

  GuiComponents();
  void render();

  void init();
  void runLater(GuiThreadTask t) {
    std::thread::id t_id = std::this_thread::get_id();
    if (t_id == this_id) {
      (*t)();
      delete t;
      return;
    }
    std::lock_guard lk(mtx);
    locked = true;
    tasks.push_back(t);
    AppState::gState->emptyEvent();
  }
  void setActivePopover(Popover* c) {
    if (!c) {
      Component* comp = dynamic_cast<Component*>(active_popover);
      if (comp && comp->canFocus())
        comp->onFocus(false);
      state->setTextReceiver(&chat_input);
    }

    active_popover = c;
    if (!c)
      return;
    Component* comp = dynamic_cast<Component*>(c);
    if (comp && comp->canFocus()) {
      comp->onFocus(true);
    }
  }
};

#endif