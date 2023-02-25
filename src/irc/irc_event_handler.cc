#include "irc_event_handler.h"
#include "../gui_components.h"
#include "../utils/notifications.h"

IrcEventHandler* create_irc_event_handler() {
#ifdef _WIN32
  ix::initNetSystem();
#endif
  IrcEventHandler* cl = new IrcEventHandler();
  return cl;
}

void IrcEventHandler::init(GuiComponents* components) {
  if (ready || connecting)
    return;
  this->components = components;
  auto* t = this;
  t->components->runLater(new std::function(
      [t]() { t->components->status_text.setData("Ready"); }));
  connecting = true;
  // infstream.zalloc = Z_NULL;
  // infstream.zfree = Z_NULL;
  // infstream.opaque = Z_NULL;
  messageBuffer = "";
}
void IrcEventHandler::addNetwork(IrcClient* client) {
  active_networks.push_back(client);
  SearchItem item;
  item.color = vec4f(1, 1, 1, 0.4);
  item.user_data = client;
  item.name = client->networkInfo.given_name;
  network_items.push_back(item);
  components->network_list.setItems(&network_items);
  client->setCallback([this](const IncomingMessage& msg, IrcClient* client,
                             UiImpact impact) { processMessage(msg, client); });
}
void IrcEventHandler::disconnect() {
  if (!active_network)
    return;
  if (active_channel_ptr)
    activateChannel(nullptr);
  IrcMessageQuit quitMsg;
  active_network->write(quitMsg);
  active_network->disconnect();
  active_network->reset();
  for (auto& entry : network_items) {
    if (reinterpret_cast<IrcClient*>(entry.user_data) == active_network) {
      entry.color = vec4f(1, 1, 1, 0.4);
    }
  }
  populateChannels(active_network);
}
void IrcEventHandler::closeAll() {
  for (auto* n : active_networks) {
    if (n->getState() == IrcClient::ConnectionState::Connected) {
      IrcMessageQuit quitMsg;
      n->write(quitMsg);
      n->disconnect();
      n->reset();
    }
  }
}
bool IrcEventHandler::isMessage(const IncomingMessage& msg) {
  if (msg.command == "PRIVMSG" || msg.command == "NOTICE")
    return true;
  if (msg.isNumericCommand && msg.numericCommand == 401)
    return true;
  return false;
}
bool IrcEventHandler::isWhoIs(const IncomingMessage& msg) {
  if (!msg.isNumericCommand)
    return false;
  auto c = msg.numericCommand;
  return c == 311 || c == 313 || c == 378 || c == 312 || c == 379 || c == 671 ||
         c == 317 || c == 318 || c == 319;
}
void IrcEventHandler::whoIsHandler(const IncomingMessage& msg,
                                   IrcClient* client) {
  auto c = msg.numericCommand;
  std::cout << "WHO IS: " << c << "\n";
  StreamReader reader(msg.parameters);
  if (reader.isNext(' '))
    reader.skip(1);
  std::string src = reader.readUntil(' ');
  reader.skip(1);
  std::string target = reader.readUntil(' ');
  reader.skip(1);
  auto& whoIsList = client->userInfos;
  if (!whoIsList.count(target)) {
    if (c == 318 && client->lastWhoIs.length() &&
        whoIsList.count(client->lastWhoIs)) {
      target = client->lastWhoIs;
    } else {
      WhoIsEntry entry;
      whoIsList[target] = entry;
    }
  }

  WhoIsEntry& e = whoIsList[target];
  if (c == 311) {
    e.nick = target;
    e.username = reader.readUntil(' ');
    reader.skip(1);
    e.host = reader.readUntil(' ');
    reader.skipUntil(':', true);
    e.realname = reader.readUntilEnd();
    client->lastWhoIs = e.nick;
  }
  if (c == 312) {
    e.server_name = reader.readUntil(' ');
    reader.skipUntil(':', true);
    e.server_info = reader.readUntilEnd();
  }
  if (c == 379) {
    reader.skipUntil('+', true);
    e.modes = reader.readUntilEnd();
  }
  if (c == 671) {
    e.secure_con = true;
  }
  if (c == 313) {
    e.op = true;
  }
  if (c == 317) {
    e.idle = std::stoi(reader.readUntil(' '));
    reader.skip(1);
    e.logonTime = std::stoi(reader.readUntil(' '));
  }
  if (c == 319) {
    reader.skipUntil(':', true);
    while (reader.rem() > 0) {
      reader.skipUntil('#', false);
      std::string channelName = reader.readUntil(' ');
      e.channels.push_back(channelName);
    }
  }
  if (c == 318) {
    client->lastWhoIs = "";
    components->userInfo.initFrom(e);
    components->runLater(new std::function(
        [this]() { components->setActivePopover(&components->userInfo); }));
  }
}
void IrcEventHandler::removeNetwork(IrcClient* client) {
  if (active_network == client) {
    active_network = nullptr;
    activateChannel(nullptr);
  }
  if (client->getState() == IrcClient::ConnectionState::Connected) {
    IrcMessageQuit quitMsg;
    client->write(quitMsg);
  }
  client->disconnect();
  client->reset();
  size_t index = 0;
  for (auto& entry : network_items) {
    if (reinterpret_cast<IrcClient*>(entry.user_data) == client) {
      break;
    }
    index++;
  }
  network_items.erase(network_items.begin() + index);
  components->network_list.setItems(&network_items);
  populateChannels(active_network);
  index = 0;
  for (auto* entry : active_networks) {
    if (entry == client) {
      break;
    }
    index++;
  }
  active_networks.erase(active_networks.begin() + index);
  delete client;
}
void IrcEventHandler::processMessage(const IncomingMessage& msg,
                                     IrcClient* client) {
  auto& joinedChannels = client->joinedChannels;
  if (msg.isNumericCommand) {
    if (msg.numericCommand == 1) {
      client->networkInfo.network_name = msg.source.host;
      components->runLater(new std::function([this, client]() {
        for (auto& entry : network_items) {
          if (reinterpret_cast<IrcClient*>(entry.user_data) == client) {
            entry.color = vec4fs(1);
            entry.name = client->networkInfo.given_name;
          }
        }
      }));
    }
    if (msg.numericCommand == 332) {
      StreamReader reader(msg.parameters);
      reader.skipUntil(' ', true);
      std::string channelName = reader.readUntil(' ');
      if (!joinedChannels.count(channelName)) {
        return;
      }
      reader.skipUntil(':', true);
      std::string topic = reader.readUntilEnd();
      IrcChannel& ch = joinedChannels[channelName];
      ch.topic = topic;
      return;
    }
    if (msg.numericCommand == 353) {
      IrcMessageNames names;
      names.fromParts(msg.parameters);
      if (!joinedChannels.count(names.channel))
        return;
      IrcChannel& ch = joinedChannels[names.channel];
      ch.users.insert(ch.users.end(), names.names.begin(), names.names.end());
    }
    if (isWhoIs(msg)) {
      whoIsHandler(msg, client);
    }
    return;
  }
  if (msg.command == "PING") {
    client->write({"PONG", msg.parameters});
    return;
  }
  if (msg.command == "NICK" && msg.forUser) {
    StreamReader reader(msg.parameters);
    reader.skip(1);
    client->nick = reader.readUntilEnd();
  }

  if (isMessage(msg)) {
    IrcMessageMsg chatMessage(msg.command, msg.isNumericCommand);
    chatMessage.source = msg.source;
    chatMessage.fromParts(msg.parameters);
    if (!joinedChannels.count(chatMessage.channel)) {
      if (chatMessage.channel == client->nick) {
        chatMessage.channel = chatMessage.source.getName();
        if (!client->networkInfo.network_name.length() ||
            chatMessage.channel == client->networkInfo.network_name)
          chatMessage.channel = client->networkInfo.given_name;
        if (!joinedChannels.count(chatMessage.channel)) {
          addChannel(client, chatMessage.channel);
        }
      } else if (chatMessage.channel == "*") {
        chatMessage.channel = client->networkInfo.given_name;
      } else {
        return;
      }
    }

    IrcChannel& ch = client->joinedChannels[chatMessage.channel];
    if(ch.type == IrcChannelType::UserChannel) {
      if(!AppState::gState->focused) {
          Notifications::sendNotification(ch.name + "@" + client->networkInfo.given_name, chatMessage.content);
      }
    }
    auto& channelMessages = ch.messages;
    channelMessages.push_back(chatMessage);
    auto* holder = message_state.add_message(chatMessage, ch);
    if (active_network == client && active_channel_ptr == &ch)
      components->runLater(new std::function([this, holder]() {
        this->components->message_list.addContent(holder);
      }));
  }
  if (msg.command == "JOIN") {
    if (msg.forUser) {
      StreamReader reader(msg.parameters);
      if (reader.isNext(':'))
        reader.skip(1);
      std::string name = reader.readUntilEnd();
      if (!joinedChannels.count(name)) {
        addChannel(client, name);
        std::cout << "added channel: " << msg.parameters << "\n";
      }
    } else {
      StreamReader reader(msg.parameters);
      if (reader.isNext(':'))
        reader.skip(1);
      std::string name = reader.readUntilEnd();
      if (joinedChannels.count(name)) {
        IrcChannel& ch = joinedChannels[name];
        std::string n = msg.source.getName();
        ch.users.push_back(n);
      }
    }
  } else if (msg.command == "PART") {
    if (msg.forUser) {
      StreamReader reader(msg.parameters);
      reader.skip(1);
      while (reader.rem() > 0) {
        std::string channelName = reader.readUntil(' ');
        reader.skip(1);
        if (channelName == client->networkInfo.network_name)
          continue;
        if (joinedChannels.count(channelName)) {
          IrcChannel* ch = &joinedChannels[channelName];
          message_state.remove_channel(ch->id);
          if (active_channel_ptr == ch)
            components->runLater(
                new std::function([this]() { activateChannel(nullptr); }));
          joinedChannels.erase(channelName);
          components->runLater(new std::function(
              [this]() { populateChannels(active_network); }));
        }
      }
    } else {
      StreamReader reader(msg.parameters);
      reader.skip(1);
      while (reader.rem() > 0) {
        std::string channelName = reader.readUntil(' ');
        reader.skip(1);
        if (channelName == client->networkInfo.network_name)
          continue;
        if (joinedChannels.count(channelName)) {
          IrcChannel* ch = &joinedChannels[channelName];
          removeFromList(ch->users, msg.source.getName());
        }
      }
    }
  }
}
void IrcEventHandler::removeFromList(std::vector<std::string>& vec,
                                     const std::string& value) {
  bool found = false;
  size_t index = 0;
  for (const auto& e : vec) {
    if (e == value || e.find(value) != std::string::npos) {
      found = true;
      break;
    }
    index++;
  }
  if (found) {
    vec.erase(vec.begin() + index);
  }
}
void IrcEventHandler::addChannel(IrcClient* client, std::string name) {
  std::cout << "added channel: " << name << "\n";
  auto& joinedChannels = client->joinedChannels;

  IrcChannelType type = client->readChannelType(name[0]);
  IrcChannel channel;
  channel.id = std::to_string(global_channel_count++);
  channel.client = client;
  channel.type = type;
  channel.name = name;
  joinedChannels[name] = channel;
  if (active_network == client)
    components->runLater(
        new std::function([this]() { populateChannels(active_network); }));
}
void IrcEventHandler::populateChannels(IrcClient* active) {
  std::cout << "populate channel\n";
  channel_items.clear();
  if (active) {
    for (std::map<std::string, IrcChannel>::iterator it =
             active->getJoinedChannels().begin();
         it != active->getJoinedChannels().end(); ++it) {
      IrcChannel& ch = it->second;
      SearchItem item;
      item.user_data = &ch;
      item.name = ch.name;
      channel_items.push_back(item);
    }
  }

  components->channel_list.setItems(&channel_items);
}

void IrcEventHandler::sendChannelMessage(std::string content) {
  if (content.length() == 0)
    return;
  StreamReader reader(content);
  if (reader.isNext('/')) {
    reader.skip(1);
    std::string command = reader.readUntil(' ');
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);
    reader.skip(1);
    if (command == "ADDSERVER") {
      std::string host = reader.readUntil(' ');
      IrcClient* client = new IrcClient(host);
      reader.skip(1);
      while (reader.rem() > 0) {
        std::string key = reader.readUntil('=');
        reader.skip(1);

        std::string value;
        if (reader.isNext('"')) {
          reader.skip(1);
          value = reader.readUntil('"');
          reader.skip(1);
        } else {
          value = reader.readUntil(' ');
        }
        reader.skip(1);
        if (key == "name") {
          client->networkInfo.given_name = value;
        }
        if (key == "nick") {
          client->nick = value;
        }
        if (key == "username") {
          client->username = value;
        }
        if (key == "realname") {
          client->realname = value;
        }
        if (key == "pass" || key == "password") {
          client->password = value;
        }
        if (key == "port") {
          client->port = std::stoi(value);
        }
        if (key == "ssl") {
          client->useTLS = value == "true";
        }
        if (key == "verify-ssl") {
          client->verifySSL = value == "true";
        }
      }
      addNetwork(client);
    } else if (command == "DELSERVER") {
      removeNetwork(active_network);
    }
    if (active_network) {
      if (command == "DISCONNECT" || command == "QUIT") {
        disconnect();
      }
      if (command == "JOIN" || command == "PART" || command == "WHOIS") {
        std::string channels = reader.readUntilEnd();
        std::vector<std::string> args = {command, channels};
        active_network->write(args);
      }
      if (command == "MSG") {
        std::string channelName = reader.readUntil(' ');
        reader.skip(1);
        query(channelName);
        while (reader.rem() > 0) {
          IrcMessageMsg msg("PRIVMSG");
          msg.content = reader.readUntil('\n');
          msg.channel = channelName;
          msg.source.nick = active_network->nick;
          msg.source.onlyHost = false;
          IrcChannel& ch = active_network->joinedChannels[msg.channel];
          auto& channelMessages = ch.messages;
          channelMessages.push_back(msg);
          auto* holder = message_state.add_message(msg, ch);
          if (active_channel_ptr == &ch)
            components->message_list.addContent(holder);
          active_network->write(msg);
          reader.skip(1);
        }
      }
      if (command == "ME") {
        while (reader.rem() > 0) {
          IrcMessageMsg msg("PRIVMSG");
          msg.content = reader.readUntil('\n');
          msg.channel = active_channel_ptr->name;
          msg.source.nick = active_network->nick;
          msg.source.onlyHost = false;
          msg.action = true;
          IrcChannel& ch = active_network->joinedChannels[msg.channel];
          auto& channelMessages = ch.messages;
          channelMessages.push_back(msg);
          auto* holder = message_state.add_message(msg, ch);
          components->message_list.addContent(holder);
          active_network->write(msg);
          reader.skip(1);
        }
      }
      if (command == "QUERY") {
        std::string v = reader.readUntilEnd();
        query(v);
      }
    }
  } else {
    if (!active_network || !active_channel_ptr)
      return;
    while (reader.rem() > 0) {
      IrcMessageMsg msg("PRIVMSG");
      msg.content = reader.readUntil('\n');
      msg.channel = active_channel_ptr->name;
      msg.source.nick = active_network->nick;
      msg.source.onlyHost = false;
      IrcChannel& ch = active_network->joinedChannels[msg.channel];
      auto& channelMessages = ch.messages;
      channelMessages.push_back(msg);
      auto* holder = message_state.add_message(msg, ch);
      components->message_list.addContent(holder);
      active_network->write(msg);
      reader.skip(1);
    }
  }

  components->chat_input.text.setData("");
  // if (!active_channel.length())
  //   return;
  // if(editMode) {
  //     DiscordMessagePayload p;
  //     p.content = content;
  //      request("/channels/" + active_channel + "/messages/" +
  //      editingMessageId, "PATCH", true, &p,
  //         nullptr, [](uint16_t http_code, bool success) {});

  //     components->chat_input.text.setData(backupData);
  //     backupData = "";
  //     editMode = false;
  //     editingMessageId = "";
  //     return;
  // }
  // DiscordMessagePayload p;

  // const auto p1 = std::chrono::system_clock::now();
  // auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
  //                p1.time_since_epoch())
  //                .count();
  // p.nonce = std::to_string(now);
  // p.content = content;
  // auto* t = this;
  // t->components->chat_input.text.setData("");
  // if(sendFiles.size()) {
  //   std::vector<HttpFileEntry> files;
  //   int c = 0;
  //   for(auto* file : sendFiles) {
  //     HttpFileEntry e = *file;
  //     e.id = std::to_string(c);
  //     files.push_back(e);
  //     DiscordMessageAttachment at;
  //     at.id = std::to_string(c);
  //     at.filename = e.name;
  //     p.attachments[std::to_string(c)] = at;
  //     c++;
  //     delete file;
  //   }
  //   for (auto& e : components->chat_input.images) {
  //       delete e.second;
  //   }
  //   components->chat_input.images.clear();
  //   sendFiles.clear();
  //   request("/channels/" + active_channel + "/messages", "POST", true,
  //   &p,files,
  //    nullptr, [t](uint16_t http_code, bool success) {});
  //   sendFiles.clear();

  // } else {
  //     request("/channels/" + active_channel + "/messages", "POST", true,
  //     &p,
  //         nullptr, [t](uint16_t http_code, bool success) {});
  // }
}
void IrcEventHandler::loadChannel(IrcChannel* ch) {
  if (active_network == ch->client && active_channel == ch->name) {
    return;
  }
  if (active_network != ch->client)
    active_network = ch->client;
  active_channel = ch->name;
  activateChannel(ch);
}
void IrcEventHandler::itemSelected(std::string type, const SearchItem* item) {
  if (type == "network") {
    IrcClient* p = reinterpret_cast<IrcClient*>(item->user_data);
    if (p->getState() == IrcClient::ConnectionState::Idle) {
      addChannel(p, p->networkInfo.given_name);
      p->connect();
      for (auto& entry : network_items) {
        if (reinterpret_cast<IrcClient*>(entry.user_data) == p) {
          entry.name = "..." + item->name;
          break;
        }
      }
      return;
    }
    active_network = p;
    populateChannels(p);
    AppState::gState->setTextReceiver(&components->channel_list);
    components->root_list_display = 3;

  } else if (type == "channel") {
    IrcChannel* ch = reinterpret_cast<IrcChannel*>(item->user_data);
    loadChannel(ch);
    AppState::gState->setTextReceiver(&components->chat_input);
  }
}
void IrcEventHandler::renderUserList() {
  if (!active_channel_ptr)
    return;
  components->user_list.initFrom(*active_channel_ptr);
  components->setActivePopover(&components->user_list);
}
void IrcEventHandler::query(std::string& name) {
  auto& joinedChannels = active_network->joinedChannels;
  if (joinedChannels.count(name))
    return;
  addChannel(active_network, name);
}
// void IrcEventHandler::onMessage(DiscordBaseMessage& msg) {
//  if (state == Handshaking && msg.opCode == DiscordOpCode::AuthResponse) {
//    DiscordInitResponseMessage initResponse;
//    state = Connected;
//    initResponse.fromJson(msg.data);
//    hb_interval = initResponse.heartbeat_interval;
//    std::cout << hb_interval << "\n";
//    auto* t = this;
//    hb_thread = new std::thread([t]() {
//      DiscordHeartbeatMessage hbMsg;
//      while (true) {
//        t->sendMessage(Heartbeat, hbMsg);
//        std::this_thread::sleep_for(std::chrono::milliseconds(t->hb_interval));
//      }
//    });
//  } else {
//    if (msg.opCode == 0) {
//      std::string evType = msg.eventType;
//      if (evType == "READY") {
//        DiscordReadyPayload p;
//        p.fromJson(msg.data);
//        onReadyPayload(p);
//        auto* t = this;
//        t->components->runLater(new std::function([t]() {
//            t->components->status_text.setData("Ready");
//            }));

//     } else if (evType == "VOICE_SERVER_UPDATE") {
//       DiscordVoiceServerUpdate update;
//       update.fromJson(msg.data);
//       vcClient.voiceServerData(update);
//     } else if (evType == "VOICE_STATE_UPDATE") {
//       DiscordVoiceConnectionUpdate update;
//       update.fromJson(msg.data);
//       bool res = vcClient.voiceStateData(update);
//       if (res) {
//         components->status_text.setData(computeVcName());
//         AppState::gState->emptyEvent();
//       }
//     } else if (evType == "CHANNEL_UPDATE") {
//       DiscordChannelPayload channel;
//       channel.fromJson(msg.data);
//       if (channel.type == DmText || channel.type == DmGroup) {
//         if (private_channels.count(channel.id)) {
//           DiscordChannelPayload* ptr = &private_channels[channel.id];
//           ptr->fromJson(msg.data);
//           for (auto& e : dm_items) {
//             if (e.user_data == ptr) {
//               e.name = getChannelName(*ptr);
//             }
//           }
//         }
//       }
//       if (channel.id == active_channel)
//         updateActiveChannel();
//     } else if (evType == "MESSAGE_CREATE") {
//       DiscordMessagePayload message;
//       message.fromJson(msg.data);
//       if (active_channel == message.channel_id) {
//         message_state.add_message(message, *active_channel_ptr);
//       } else if (private_channels.count(message.channel_id)) {
//         DiscordChannelPayload& ch = private_channels[message.channel_id];
//         message_state.add_message(message, ch);
//         uint32_t index = 0;
//         DiscordChannelPayload* chPtr = &ch;
//         SearchItem item;
//         bool f = false;
//         for (auto& e : dm_items) {
//             if (e.user_data == chPtr) {
//                 item = e;
//                 f = true;
//                 break;
//             }

//             index++;
//         }
//         if (f) {
//             dm_items.erase(dm_items.begin() + index);
//             dm_items.insert(dm_items.begin(), item);
//           }
//       }
//       if (active_channel == message.channel_id) {
//           auto* t = this;
//           std::string id = message.id;
//           components->runLater(new std::function([t, id]() {
//               t->components->message_list.addContent(
//                   t->message_state.message_index[id]);
//               }));
//       }
//     } else if (evType == "MESSAGE_UPDATE") {
//       DiscordMessagePayload message;
//       message.fromJson(msg.data);
//       if (message_state.message_index.count(message.id)) {
//         MessageHolder* h = message_state.message_index[message.id];
//         h->message = message;
//         if (active_channel == message.channel_id) {
//           auto* t = this;
//           components->runLater(new std::function([t, h]() {
//             auto& l = t->components->message_list;
//             l.updateEntry(h);
//           }));
//         }
//       }
//     } else if (evType == "MESSAGE_DELETE") {
//       DiscordMessageDeletePayload* deleter =
//           new DiscordMessageDeletePayload();
//       deleter->fromJson(msg.data);
//       if (active_channel == deleter->channel_id) {
//         auto* t = this;
//         components->runLater(new std::function([t, deleter]() {
//           t->components->message_list.removeEntry(
//               t->message_state.message_index[deleter->id]);
//           t->message_state.remove_message(deleter->id);
//           delete deleter;
//         }));
//       } else {
//         message_state.remove_message(deleter->id);
//         delete deleter;
//       }
//     } else if (evType == "MESSAGE_REACTION_ADD") {
//       DiscordMessageReactionAddPayload add;
//       add.fromJson(msg.data);
//       if (add.emote_id.length() &&
//           message_state.message_index.count(add.message_id)) {
//         MessageHolder* holder =
//         message_state.message_index[add.message_id]; DiscordMessagePayload&
//         msg = holder->message; if (msg.reactions.count(add.emote_id)) {
//           (&msg.reactions[add.emote_id])->count++;
//         } else {
//           DiscordMessageReactionPayload reaction;
//           reaction.count = 1;
//           reaction.emote_id = add.emote_id;
//           reaction.emote_name = add.emote_name;
//           reaction.me = add.user_id == user.id;
//           msg.reactions[reaction.emote_id] = reaction;
//         }
//         if (active_channel == add.channel_id)
//           AppState::gState->emptyEvent();
//       }
//     } else if (evType == "CALL_CREATE") {
//       if (startedDmCall) {
//         DiscordCallRingRequest req;
//         request("/channels/" + active_channel + "/call/ring", "POST", true,
//                 &req, nullptr, [](uint16_t http_code, bool success) {});
//         startedDmCall = false;
//       }
//     } else if (evType == "CHANNEL_CREATE") {
//       DiscordChannelPayload p;
//       p.fromJson(msg.data);
//       if (p.type == DmText || p.type == DmGroup) {
//         private_channels[p.id] = p;
//         SearchItem item;
//         item.user_data = &private_channels[p.id];
//         item.name = getChannelName(p);
//         dm_items.insert(dm_items.begin(), item);
//         AppState::gState->emptyEvent();
//       } else if (p.type == VcChannel || p.type == TextChannel) {
//         (&guilds[p.guild_id])->channels[p.id] = p;
//       }
//     } else if (evType == "GUILD_CREATE") {
//       DiscordGuildPayload guild;
//       guild.fromJson(msg.data);
//       guilds[guild.id] = guild;
//       SearchItem item;
//       item.user_data = &guilds[guild.id];
//       item.name = guild.name;
//       guild_items.insert(guild_items.begin(), item);
//       components->guilds_list.recompute();
//       AppState::gState->emptyEvent();
//     } else if (evType == "GUILD_DELETE") {
//       std::string id = msg.data["id"];
//       if (guilds.count(id)) {
//         DiscordGuildPayload& g = guilds[id];
//         uint32_t index = -1;
//         for (int i = 0; i < guild_items.size(); ++i) {
//           if (guild_items[i].user_data == &g) {
//             index = i;
//             break;
//           }
//         }
//         if (index != -1) {
//           guild_items.erase(guild_items.begin() + index);
//           components->guilds_list.recompute();
//         }
//         for (auto& ch_raw : g.channels) {
//           auto& ch = ch_raw.second;

//           if (active_channel == ch.id) {
//             channel_items.clear();
//             components->channel_list.recompute();

//             active_channel = "";
//             active_channel_ptr = nullptr;
//             auto* t = this;
//             std::string* ptr = new std::string(ch.id);
//             components->runLater(new std::function([t, ptr]() {
//               t->components->message_list.clearList();
//               t->message_state.remove_channel(*ptr);
//               delete ptr;
//               t->components->header_text.setData("");
//             }));

//           } else {
//             if (message_state.state.count(ch.id))
//               message_state.remove_channel(ch.id);
//           }
//         }
//         guilds.erase(g.id);
//         AppState::gState->emptyEvent();
//       }
//     } else if (evType == "CHANNEL_DELETE") {
//       DiscordChannelPayload ch;
//       ch.fromJson(msg.data);

//       if (ch.guild_id.size()) {
//         auto& guild = guilds[ch.guild_id];
//         DiscordChannelPayload& lChannel = guild.channels[ch.id];
//         uint32_t index = -1;
//         for (int i = 0; i < channel_items.size(); ++i) {
//           auto& e = channel_items[i];
//           if (e.user_data == &lChannel) {
//             index = i;
//             break;
//           }
//         }
//         if (index != -1) {
//           channel_items.erase(channel_items.begin() + index);
//           components->channel_list.recompute();
//         }
//         guild.channels.erase(ch.id);
//       } else if (ch.type == DmText || ch.type == DmGroup) {
//         auto& lChannel = private_channels[ch.id];
//         uint32_t index = -1;
//         for (int i = 0; i < dm_items.size(); ++i) {
//           auto& e = dm_items[i];
//           if (e.user_data == &lChannel) {
//             index = i;
//             break;
//           }
//         }
//         if (index != -1) {
//           dm_items.erase(channel_items.begin() + index);
//           components->dm_list.recompute();
//         }
//         private_channels.erase(ch.id);
//       }

//       if (active_channel == ch.id) {
//         active_channel = "";
//         active_channel_ptr = nullptr;
//         auto* t = this;
//         std::string* ptr = new std::string(ch.id);
//         components->runLater(new std::function([t, ptr]() {
//           t->components->message_list.clearList();
//           t->message_state.remove_channel(*ptr);
//           delete ptr;
//           t->components->header_text.setData("");
//         }));
//       } else {
//         message_state.remove_channel(ch.id);
//       }
//     } else if(evType == "READY_SUPPLEMENTAL") {
//       DiscordSuplementalReadyPayload r;
//       r.fromJson(msg.data);
//       presences = r.presences;
//     }
//   }
// }
//}
void IrcEventHandler::updateActiveChannel() {
  // auto* t = this;
  // components->runLater(new std::function([t]() {
  //   t->components->header_text.setData(
  //       t->getChannelName(*t->active_channel_ptr));
  // }));
}
// void IrcEventHandler::sendEvent(std::string name, DiscordMessage& msg) {
//    if (!connecting)
//      return;
//    json d = msg.getJson();
//    DiscordBaseMessage m = {Ev, d, name};
//    json toSend = m;
//    std::cout << toSend << "\n";
//    m_web_socket.send(toSend.dump());
//  };

// void DiscordClient::loadChannel(DiscordChannelPayload* channel) {
//   std::string channel_id = channel->id;
//   std::cout << channel_id << "\n";
//   if (active_channel == channel_id)
//     return;

//   bool needsLoad = !message_state.state.count(channel_id) ||
//                    (&message_state.state[channel_id])->loaded == false;
//   std::cout << needsLoad << ":" << channel_id << "\n";
//   if (needsLoad) {
//     auto* t = this;
//     DiscordMessageListStruct* list = new DiscordMessageListStruct();
//     request("/channels/" + channel_id + "/messages?limit=50", "GET", false,
//             nullptr, list,
//             [list, channel, t](uint16_t http_code, bool success) {
//               if (!success) {
//                 delete list;
//                 t->components->runLater(new std::function(
//                     [t, channel]() { t->activateChannel(channel); }));
//                 return;
//               }
//               std::vector<DiscordMessagePayload>& messages =
//               list->messages; t->message_state.load_channel(messages,
//               *channel);

//               delete list;

//               t->components->runLater(new std::function(
//                   [t, channel]() { t->activateChannel(channel); }));
//             });
//   } else {
//     activateChannel(channel);
//   }
//}

void IrcEventHandler::activateChannel(IrcChannel* ch) {
  if (ch == nullptr) {
    components->header_text.setData("");
    components->message_list.clearList();
    active_channel = "";
    active_channel_ptr = nullptr;
    return;
  }
  std::string id = ch->id;
  std::cout << "active channel: " << ch->name << "\n";
  IrcChannel& ref = *ch;
  components->header_text.setData(ref.name);
  components->message_list.clearList();
  ChannelState& state = message_state.state[id];
  for (auto* msg : state.messages) {
    components->message_list.addContent(msg);
  }
  components->message_list.scrollEnd();
  active_channel = ref.name;
  active_channel_ptr = ch;
}
// void IrcEventHandler::sendMessage(DiscordOpCode op, DiscordMessage& msg) {
//   if (!connecting)
//     return;
//   json d = msg.getJson();
//   DiscordBaseMessage m = {op, d, ""};
//   json toSend = m;
//   std::cout << toSend << "\n";
//   m_web_socket.send(toSend.dump());
// };
// void IrcEventHandler::request(std::string path,
//                             std::string method,
//                             bool hasBody,
//                             DiscordMessage* body,
//                             DiscordMessage* out,
//                             const HttpResultCallback& callback) {
//   std::cout << "http req: " << method << ":" << path << "\n";
//   ix::WebSocketHttpHeaders headers;
//   if (method != "GET")
//     headers["Content-Type"] = "application/json";
//   headers["User-Agent"] =
//       "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 "
//       "(KHTML, like Gecko) discord/0.0.266 Chrome/91.0.4472.164 "
//       "Electron/13.6.6 Safari/537.3";
//   std::string url = api_base + path;
//   auto req = httpClient.createRequest(url, method);
//   req->extraHeaders = headers;
//   if (hasBody) {
//     json b = body->getJson();
//     req->body = b.dump();
//   }
//   httpClient.performRequest(
//       req, [out, callback](const ix::HttpResponsePtr& response) {
//         auto errorCode = response->errorCode;
//         auto responseCode = response->statusCode;
//         bool success = errorCode == ix::HttpErrorCode::Ok;
//         if (success && responseCode == 200) {
//           auto payload = response->body;
//           if (payload.length() && out != nullptr) {
//             json parsed = json::parse(payload);
//             out->fromJson(parsed);
//           }
//         }
//         callback(responseCode, success);
//       });
// }
// void IrcEventHandler::request(std::string path,
//                             std::string method,
//                             bool hasBody,
//                             DiscordMessage* body,
//                             std::vector<HttpFileEntry>& files,
//                             DiscordMessage* out,
//                             const HttpResultCallback& callback) {
//   std::cout << "http req multipart: " << method << ":" << path << "\n";
//   ix::WebSocketHttpHeaders headers;
//   json b = body->getJson();
//   std::string jsonPayload = b.dump();
//   FormData formData = FormData::generate(files, jsonPayload);
//   if (method != "GET")
//     headers["Content-Type"] =
//         "multipart/form-data; boundary=----" + formData.boundary;
//   headers["User-Agent"] =
//       "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 "
//       "(KHTML, like Gecko) discord/0.0.266 Chrome/91.0.4472.164 "
//       "Electron/13.6.6 Safari/537.3";
//   std::string url = api_base + path;
//   auto req = httpClient.createRequest(url, method);
//   req->extraHeaders = headers;
//   if (hasBody) {
//     req->body = formData.data;
//   }
//   httpClient.performRequest(
//       req, [out, callback](const ix::HttpResponsePtr& response) {
//         auto errorCode = response->errorCode;
//         auto responseCode = response->statusCode;
//         bool success = errorCode == ix::HttpErrorCode::Ok;
//         if (success && responseCode == 200) {
//           auto payload = response->body;
//           if (payload.length() && out != nullptr) {
//             json parsed = json::parse(payload);
//             out->fromJson(parsed);
//           }
//         }
//         callback(responseCode, success);
//       });
// }
void IrcEventHandler::fetchMore() {
  // if (!active_channel.length() || fetching ||
  //     components->message_list.msg_load_ptr != nullptr) {
  //   return;
  // }
  // ChannelState& ch_state = message_state.state[active_channel];

  // if (ch_state.reached_end || !ch_state.loaded)
  //   return;
  // std::cout << "invoked fetch more\n";
  // fetching = true;
  // std::string first = ch_state.messages[0]->id;
  // auto* t = this;
  // DiscordMessageListStruct* list = new DiscordMessageListStruct();
  // request(
  //     "/channels/" + active_channel + "/messages?limit=50&before=" + first,
  //     "GET", false, nullptr, list, [t, list](uint16_t http_code, bool
  //     success) {
  //       if (!success)
  //         return;
  //       std::vector<DiscordMessagePayload>& messages = list->messages;
  //       t->message_state.state[t->active_channel].reached_end =
  //           messages.size() < 50;
  //       auto out =
  //           t->message_state.prepend_messages(messages,
  //           *t->active_channel_ptr);
  //       std::vector<MessageHolder*>* ptr = new
  //       std::vector<MessageHolder*>(out);
  //       t->components->message_list.msg_load_ptr = ptr;
  //       t->fetching = false;
  //       delete list;
  //       AppState::gState->emptyEvent();
  //     });
}
void IrcEventHandler::tryDelete() {
  // auto mlist = components->message_list;
  // if (mlist.selected_index != -1) {
  //   auto index = mlist.messages.size() - mlist.selected_index - 1;
  //   RenderMessage* m = mlist.messages[index];
  //   DiscordMessagePayload& msg = m->m_holder->message;
  //   request("/channels/" + msg.channel_id + "/messages/" + msg.id,
  //   "DELETE",
  //           false, nullptr, nullptr, [](uint16_t http_code, bool success)
  //           {});
  // }
}
void IrcEventHandler::tryEdit() {
  // if(editMode)
  //   return;
  //   auto mlist = components->message_list;
  // if (mlist.selected_index != -1) {
  //   auto index = mlist.messages.size() - mlist.selected_index - 1;
  //   RenderMessage* m = mlist.messages[index];
  //   DiscordMessagePayload& msg = m->m_holder->message;
  //   editMode = true;
  //   editingMessageId = msg.id;
  //   backupData = components->chat_input.text.getUtf8Value();
  //   components->chat_input.text.setData(msg.content);
  //   AppState::gState->setTextReceiver(&components->chat_input);
  // }
}
void IrcEventHandler::cancelEdit() {
  if (!editMode)
    return;
  editMode = false;
  editingMessageId = "";
  components->chat_input.text.setData(backupData);
  backupData = "";
}
IrcEventHandler::IrcEventHandler() : httpClient(true) {}
