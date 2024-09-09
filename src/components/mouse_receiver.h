#ifndef DEC_MOUSE_RECEIVER_H
#define DEC_MOUSE_RECEIVER_H

class MouseReceiver {
public:
  virtual void onMousePress(double x, double y, int button, int action) = 0;
  virtual void onMouseWheel(double xoffset, double yoffset) = 0;
};

#endif