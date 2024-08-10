#ifndef DEC_MESSAGE_STATE
#define DEC_MESSAGE_STATE
#include <map>
#include <vector>
#include "../irc/irc_messages.hpp"
#include "../irc/irc_client.h"
#include "../../third-party/json/json.hpp"

using json = nlohmann::json;

enum MessageSendState { Pending, Send, Failed };
class MessageHolder {
 public:
  IrcMessageMsg message;
  MessageSendState msg_state = Send;
  MessageHolder(IrcMessageMsg p);
};
class ChannelState {
 public:
  std::string channel_name;
  bool loaded = false;
  bool received_message = false;
  bool reached_end = false;
  std::vector<MessageHolder*> messages;
};
class MessageState {
private:
  std::map<std::string, ChannelState> state;
 public:
  std::map<std::string, MessageHolder*> message_index;
  MessageHolder* add_message(IrcMessageMsg message, IrcChannel& channel);
  ChannelState& get(IrcChannel& channel);
  void persistChannel(IrcChannel* channel);
  // void load_channel(std::vector<IrcMessageMsg>& messages,
  //                   IrcChannel& channel);
  void remove_channel(std::string id);
  // void remove_message(std::string id);
  // std::vector<MessageHolder*> prepend_messages(
  //     std::vector<IrcMessageMsg>& messages,
  //     DiscordChannelPayload& channel);
};
#endif