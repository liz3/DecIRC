#ifndef DEC_IRC_POOL_H
#define DEC_IRC_POOL_H

#include <vector>
#include "irc_client.h"
#include "../irc/image_cache.h"

class IrcPool {
 public:
  static IrcPool* createPool();

 private:
  std::vector<IrcClient> clients;
};
#endif