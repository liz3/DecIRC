#ifndef DEC_CHAT_INPUT
#define DEC_CHAT_INPUT
#include "message_list.h"
#include "text_field.h"
class ChatInput : public TextField {
 public:
  MessageList* list = nullptr;
  ChatInput() {}
  void onFocus(bool focus) override {
    box.renderCursor = focus;
    if (list)
      list->hasFocus = focus;
  }
};
#endif