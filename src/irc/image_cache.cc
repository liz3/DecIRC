#include "image_cache.h"
#include <iostream>
#include "../../third-party/png/lodepng.h"
#include "../gui_components.h"

ImageCache::ImageCache() : httpClient(true) {}

void ImageCache::fetchImage(std::string url,
                            void* ptr,
                            const ImageCacheCallback& cb) {
  const auto p1 = std::chrono::system_clock::now();
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                 p1.time_since_epoch())
                 .count();
  if (images.count(url)) {
    ImageCacheEntry& img = images[url];
    if (now - img.last_fetch < 1000 * 60 * 25) {
      cb(true, &img);
      return;
    }
  }
  auto req = httpClient.createRequest(url, "GET");
  auto* t = this;
  if (pending.count(ptr)) {
    pending[ptr]++;
  } else {
    pending[ptr] = 1;
  }
  std::cout << "fetching image: " << url << "\n";
  httpClient.performRequest(req, [t, cb, url,
                                  ptr](const ix::HttpResponsePtr& response) {
    bool valid = true;
    auto point = std::find(t->dead.begin(), t->dead.end(), ptr);
    if (point != t->dead.end()) {
      valid = false;
      t->pending[ptr]--;
      if (t->pending[ptr] == 0) {
        t->dead.erase(point);
        t->pending.erase(ptr);
      }
    }
    auto errorCode = response->errorCode;
    auto responseCode = response->statusCode;
    bool success = errorCode == ix::HttpErrorCode::Ok && responseCode == 200;
    if (!success) {
      if (valid)
        cb(false, nullptr);
    } else {
      ImageCacheEntry entry;
      std::string contentType = response->headers["Content-Type"];
      std::cout << "img type: " << contentType << "\n";
      ImageType type;
      if (contentType == "image/png") {
        type = Png;

      } else if (contentType == "image/webp") {
        type = Webp;
      } else if (contentType == "image/jpeg" || contentType == "image/jpg") {
        type = Jpg;
      } else if (contentType == "image/gif") {
        type = Gif;
      }
      std::string& p = response->body;
      std::vector<uint8_t> in(p.c_str(), p.c_str() + p.length());
      entry.data = in;
      const auto p1 = std::chrono::system_clock::now();
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                     p1.time_since_epoch())
                     .count();
      entry.last_fetch = now;
      entry.type = type;
      t->images[url] = entry;
      if (valid)
        cb(true, &t->images[url]);
    }
  });
}
Image* ImageCache::getEmote(std::string id) {

  return nullptr;
}
void ImageCache::reportDead(void* ptr) {
  if (pending.count(ptr)) {
    dead.push_back(ptr);
  }
}