#ifndef DEC_TEXT_FIELD
#define DEC_TEXT_FIELD
#include "../rendering/text.h"
#include "text_receiver.h"
#include <functional>
#include "../rendering/text_box.h"
using OnEnterCallback = std::function<void(std::string content)>;
class TextField : public TextReceiver {
 public:
  TextField();

  bool canFocus() override;
  void onFocus(bool focus) override;
  void render(float x, float y, float w, float h) override;
  void onEnter() override;
  void onCodePoint(int32_t cp) override;
  void onKey(GLFWwindow* window,
             int key,
             int scancode,
             int action,
             int mods) override;
  void addText(std::string text) override;
  std::string getText() override;
  void setOnChangeCb(const OnEnterCallback& cb);
  TextWithState text;
  TextBox box;
  OnEnterCallback enter_cb;

 private:
  OnEnterCallback on_change_cb;
  bool has_change_cb = false;
};
#endif