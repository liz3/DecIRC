#include "image_overlay.h"
#include "../rendering/box.h"
#include "../rendering/image.h"
#include <iostream>

void ImageOverlay::render(float width, float height) {
  if (!image->valid)
    return;
  Box::render(-width, height, width * 2, -(height * 2), vec4f(0, 0, 0, 0.6));
  float t = width;
  if (height < width) {
    t = height;
  }
  if (image->height > image->width) {
    float target = t - 10;
    if (target > image->height)
      target = image->height;
    float scale = target / image->height;
    image->render(-((image->width / 2) * scale), -((image->height / 2) * scale),
                  scale);

  } else {
    float target = t - 10;
    if (target > image->width)
      target = image->width;
    float scale = target / image->width;
    image->render(-((image->width / 2) * scale), -((image->height / 2) * scale),
                  scale);
  }
}

void ImageOverlay::initFrom(Image* src) {

  if(!src->decoded) {
    src->init();
  }
  if(image) {
    delete image;
  }
  image = new Image(src->data, src->width, src->height);
}