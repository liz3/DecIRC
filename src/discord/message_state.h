#ifndef DEC_MESSAGE_STATE
#define DEC_MESSAGE_STATE
#include <map>
#include <vector>
#include "structs.h"

enum MessageSendState { Pending, Send, Failed };
class MessageHolder {
 public:
  DiscordMessagePayload message;
  std::string id;
  MessageSendState msg_state = Send;
  MessageHolder(DiscordMessagePayload p);
};
class ChannelState {
 public:
  std::string id;
  bool loaded = false;
  bool reached_end = false;
  std::vector<MessageHolder*> messages;
};
class MessageState {
 public:
  std::map<std::string, MessageHolder*> message_index;
  std::map<std::string, ChannelState> state;
  void add_message(DiscordMessagePayload message,
                   DiscordChannelPayload& channel);
  void load_channel(std::vector<DiscordMessagePayload>& messages,
                    DiscordChannelPayload& channel);
  void remove_channel(std::string id);
  void remove_message(std::string id);
  std::vector<MessageHolder*> prepend_messages(
      std::vector<DiscordMessagePayload>& messages,
      DiscordChannelPayload& channel);
};
#endif