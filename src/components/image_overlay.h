#ifndef DEC_IMAGE_OVERLAY
#define DEC_IMAGE_OVERLAY
class Image;
#include "popover.h"

class ImageOverlay : public Popover {
 public:
  void render(float width, float height) override;
  Image* image = nullptr;
  void initFrom(Image* src);
};
#endif