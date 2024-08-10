#include "message_state.h"
#include "../gui_components.h"


MessageHolder* MessageState::add_message(IrcMessageMsg message,
                                         IrcChannel& channel) {

  ChannelState& channel_state = get(channel);
  channel_state.received_message = true;
  MessageHolder* holder = new MessageHolder(message);

  channel_state.messages.push_back(holder);
  if (!AppState::gState->focused) {
    persistChannel(&channel);
  }
  return holder;
}
void MessageState::persistChannel(IrcChannel* channel) {
  if(!state.count(channel->id))
    return;
  ChannelState& channel_state = state[channel->id];
  int cache_size = AppState::gState->config.getCacheSize();
  if(channel_state.received_message) {
    int diff = channel_state.messages.size() - cache_size;
    if(diff < 0)
      diff = 0;
    json arr = json::array();
    for(int i = diff; i < channel_state.messages.size(); i++) {
      json m = channel_state.messages[i]->message.toJson();
      arr.push_back(m);
    }
    AppState::gState->config.saveCache(channel->client->networkInfo.given_name, channel->name, arr);
    channel_state.received_message = false;
  }
}
ChannelState& MessageState::get(IrcChannel& channel) {
  if(state.count(channel.id))
    return state[channel.id];
    ChannelState channel_state;
    channel_state.channel_name = channel.name;
    json j = AppState::gState->config.loadCache(channel.client->networkInfo.given_name, channel.name);
    if(j.size()) {
      for(auto& jEntry : j){
        IrcMessageMsg msg("");
        msg.fromJson(jEntry);
        MessageHolder* holder = new MessageHolder(msg);
        channel_state.messages.push_back(holder);
        channel.messages.push_back(msg);
      }
    }
    state[channel.id] = channel_state;
    return state[channel.id];
}
void MessageState::remove_channel(std::string id) {
  if (!state.count(id))
    return;
  ChannelState& channel = state[id];
  for (auto* message : channel.messages) {
    delete message;
  }
  state.erase(id);
}
MessageHolder::MessageHolder(IrcMessageMsg p) : message(p) {}
// void MessageState::load_channel(std::vector<DiscordMessagePayload>& messages,
//                                 DiscordChannelPayload& channel) {
//   if (state.count(channel.id) && (&state[channel.id])->loaded) {
//     return;
//   }
//   if (!state.count(channel.id)) {
//     ChannelState channel_state;
//     state[channel.id] = channel_state;
//   }
//   ChannelState& channel_state = state[channel.id];

//   for (auto msg : messages) {
//       if (message_index.count(msg.id))
//           continue;
//     MessageHolder* holder = new MessageHolder(msg);
//     channel_state.messages.insert(channel_state.messages.begin(), holder);
//     message_index[msg.id] = holder;
//   }

//   channel_state.loaded = true;
// }
// std::vector<MessageHolder*> MessageState::prepend_messages(
//     std::vector<IrcMessageMsg>& messages,
//     DiscordChannelPayload& channel) {
//   std::vector<MessageHolder*> ret;
//   if (!state.count(channel.id)) {
//     return ret;
//   }
//   ChannelState& channel_state = state[channel.id];
//   for (auto msg : messages) {
//     MessageHolder* holder = new MessageHolder(msg);
//     channel_state.messages.insert(channel_state.messages.begin(), holder);
//     ret.push_back(holder);
//   }
//   return ret;
// }