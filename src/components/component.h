#ifndef DEC_COMPONENT
#define DEC_COMPONENT
class Component {
 public:
  virtual bool canFocus() = 0;
  virtual void onFocus(bool focus) = 0;
  virtual void render(float x, float y, float w, float h) = 0;
};
#endif