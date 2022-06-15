#ifndef DEC_IMAGE_CACHE
#define DEC_IMAGE_CACHE
#include <ixwebsocket/IXHttpClient.h>
#include <map>
#include <vector>
enum ImageType { Png, Webp, Jpg };
class Image;
class ImageCacheEntry {
 public:
  std::vector<uint8_t> data;
  ImageType type;
  uint64_t last_fetch;
};
using ImageCacheCallback = std::function<void(bool, ImageCacheEntry*)>;

class ImageCache {
 public:
  ImageCache();
  ix::HttpClient httpClient;
  std::map<void*, uint32_t> pending;
  std::vector<void*> dead;
  std::map<std::string, bool> pending_emotes;
  std::map<std::string, Image*> emotes;
  Image* getEmote(std::string id);
  std::map<std::string, ImageCacheEntry> images;
  void fetchImage(std::string url, void* ptr, const ImageCacheCallback& cb);
  void reportDead(void* ptr);
};
#endif