#include "irc_pool.h"

IrcPool* IrcPool::createPool() {
  return new IrcPool();
}