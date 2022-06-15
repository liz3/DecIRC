#ifndef DEC_GUI_COMPONENTS
#define DEC_GUI_COMPONENTS
class AppState;

#include "./components/chat_input.h"
#include "./components/message_list.h"
#include "./components/search_list.h"
#include "./components/text_field.h"
#include "./components/user_info.h"

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
  SearchList guilds_list;
  TextBox header_comp;
  TextWithState header_text;
  TextBox status_comp;
  TextWithState status_text;
  std::vector<GuiThreadTask> tasks;
  MessageList message_list;
  std::thread::id this_id = std::this_thread::get_id();
  bool locked = false;
  Component* active_popover = nullptr;

  UserInfo userInfo;
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
    mtx.lock();
    locked = true;
    std::cout << "task queued\n";
    tasks.push_back(t);
    AppState::gState->emptyEvent();
  }
  void setActivePopover(Component* c) { active_popover = c; }
};

#endif