#ifndef DEC_TEXT_RECEIVER
#define DEC_TEXT_RECEIVER
class GLFWwindow;
#include <string>
#include "component.h"
class TextReceiver : public Component {
 public:
  virtual void onEnter() = 0;
  virtual void onCodePoint(int32_t cp) = 0;
  virtual void onKey(GLFWwindow* window,
                     int key,
                     int scancode,
                     int action,
                     int mods) = 0;
};
#endif