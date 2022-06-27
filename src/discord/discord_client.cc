#include "discord_client.h"
#include "../gui_components.h"

void to_json(json& j, const DiscordBaseMessage& o) {
  j = json{{"op", (int32_t)o.opCode}, {"d", o.data}};
  if (o.eventType.length())
    j["t"] = o.eventType;
  else
    j["t"] = nullptr;
}

void from_json(const json& j, DiscordBaseMessage& o) {
  j.at("op").get_to(o.opCode);
  j.at("d").get_to(o.data);
  if (j.contains("t") && j["t"].is_string())
    j.at("t").get_to(o.eventType);
  else
    o.eventType = "";
}

DiscordClient* create_discord_client(std::string token) {
#ifdef _WIN32
  ix::initNetSystem();
#endif
  DiscordClient* cl = new DiscordClient();
  cl->m_token = token;
  cl->vcClient.d_token = token;
  auto& websocket = cl->m_web_socket;
  websocket.setUrl(
      "wss://gateway.discord.gg/?encoding=json&v=9&compress=zlib-stream");
  return cl;
}

void DiscordClient::init(GuiComponents* components) {
  if (ready || connecting)
    return;
  this->components = components;
  auto* t = this;
  t->components->runLater(new std::function([t]() {
      if(!t->m_token.length())
         t->components->status_text.setData("Token invalid");
        else
       t->components->status_text.setData("Loading");
      }));
  if(!m_token.length())
    return;
  connecting = true;
  infstream.zalloc = Z_NULL;
  infstream.zfree = Z_NULL;
  infstream.opaque = Z_NULL;
  messageBuffer = "";
  m_web_socket.setOnMessageCallback([t](const ix::WebSocketMessagePtr& msg) {
    if (msg->type == ix::WebSocketMessageType::Message) {
      t->onWsMessage(msg);
    } else if (msg->type == ix::WebSocketMessageType::Open) {
      std::cout << "invoked open\n";
      t->onWsReady();
    } else if (msg->type == ix::WebSocketMessageType::Error) {
      // Maybe SSL is not configured properly
      std::cout << "Connection error: " << msg->errorInfo.reason << std::endl;
    }
  });
  m_web_socket.start();
}

void DiscordClient::onWsReady() {
  if (state != Handshake)
    return;
  state = Handshaking;
  DiscordTokenAuthMessage a(m_token);
  sendMessage(DiscordOpCode::ClientAuth, a);
}
void DiscordClient::onWsMessage(const ix::WebSocketMessagePtr& msg) {
  messageBuffer += msg->str;
  if (messageBuffer.length() >= 4) {
    uint8_t* data = (uint8_t*)messageBuffer.c_str();
    data += (messageBuffer.length() - 4);
    uint32_t* numPtr = reinterpret_cast<uint32_t*>(data);
    uint32_t n = *numPtr;
    if (n == 0xffff0000) {
      if (!zInit) {
        inflateInit(&infstream);
        zInit = true;
      }
      int size = 1024;
      uint8_t* c = new uint8_t[size];
      infstream.avail_in = (uInt)messageBuffer.length();
      infstream.next_in = (Bytef*)messageBuffer.c_str();
      infstream.avail_out = (uInt)1024;
      infstream.next_out = (Bytef*)c;
      uint32_t dataSize = 0;
      while (true) {
        int out = inflate(&infstream, Z_SYNC_FLUSH);
        if (out == Z_STREAM_END || out == Z_BUF_ERROR || out == -3)
          break;
        if (out == Z_OK && infstream.avail_out == 0) {
          dataSize += 1024;
          size += 1024;
          infstream.avail_out += 1024;

          c = (uint8_t*)realloc(c, size);
          infstream.next_out = (Bytef*)&c[size - 1024];
        } else if (out == Z_OK) {
          dataSize += 1024 - (infstream.avail_out);
          break;
        }
      }
      std::string dataStr = std::string(c, c + (dataSize));
      if (!dataStr.length()) {
          delete[] c;
          return;
      }
      json j = json::parse(dataStr);
      auto p = j.get<DiscordBaseMessage>();
      onMessage(p);

      messageBuffer = "";
      delete[] c;
    }
  }
}
void DiscordClient::startDmCall() {
  if (!active_channel_ptr || active_channel_ptr->type != DmText)
    return;
  initVc("", active_channel);
  startedDmCall = true;
}
void DiscordClient::initVc(std::string guild_id, std::string channel_id) {
  std::cout << "vc init: " << guild_id << ":" << channel_id << "\n";

  if (vcClient.state != -1) {
    vcClient.disconnect();
    DiscordVoiceStateUpdate state;
    state.guild_id = "";
    state.channel_id = "";
    sendMessage(VoiceStateUpdate, state);
    components->status_text.setData("Ready");
    return;
  }
  vcClient.init(guild_id, channel_id);
  DiscordVoiceStateUpdate state;
  state.guild_id = guild_id;
  state.channel_id = channel_id;
  sendMessage(VoiceStateUpdate, state);

  components->status_text.setData(computeVcName());
}
void DiscordClient::onReadyPayload(DiscordReadyPayload p) {
  readyP = p;
  user = p.user;
  vcClient.user = &user;
  private_channels = p.private_channels;
  guilds = p.guilds;
  {
    dm_items.clear();
    for (std::map<std::string, DiscordChannelPayload>::iterator it =
             private_channels.begin();
         it != private_channels.end(); ++it) {
      DiscordChannelPayload& channel = it->second;
      SearchItem item;
      item.user_data = &channel;
      item.name = getChannelName(channel);
      dm_items.push_back(item);
    }
    components->dm_list.setItems(&dm_items);
  }

  {
    guild_items.clear();
    for (std::map<std::string, DiscordGuildPayload>::iterator it =
             guilds.begin();
         it != guilds.end(); ++it) {
      SearchItem item;
      item.user_data = &it->second;
      item.name = it->second.name;
      guild_items.push_back(item);
    }
    components->guilds_list.setItems(&guild_items);
  }

  AppState::gState->emptyEvent();
}
void DiscordClient::sendChannelMessage(std::string content) {
  if (!active_channel.length())
    return;
  if(editMode) {
      DiscordMessagePayload p;
      p.content = content;
       request("/channels/" + active_channel + "/messages/" + editingMessageId, "PATCH", true, &p,
          nullptr, [](uint16_t http_code, bool success) {});

      components->chat_input.text.setData(backupData);
      backupData = "";
      editMode = false;
      editingMessageId = "";
      return;
  }
  DiscordMessagePayload p;

  const auto p1 = std::chrono::system_clock::now();
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                 p1.time_since_epoch())
                 .count();
  p.nonce = std::to_string(now);
  p.content = content;
  auto* t = this;
  t->components->chat_input.text.setData("");
  if(sendFiles.size()) {
    std::vector<HttpFileEntry> files;
    int c = 0;
    for(auto* file : sendFiles) {
      HttpFileEntry e = *file;
      e.id = std::to_string(c);
      files.push_back(e);
      DiscordMessageAttachment at;
      at.id = std::to_string(c);
      at.filename = e.name;
      p.attachments[std::to_string(c)] = at;
      c++;
      delete file;
    }
    for (auto& e : components->chat_input.images) {
        delete e.second;
    }
    components->chat_input.images.clear();
    sendFiles.clear();
    request("/channels/" + active_channel + "/messages", "POST", true, &p,files,
     nullptr, [t](uint16_t http_code, bool success) {});
    sendFiles.clear();

  } else {
      request("/channels/" + active_channel + "/messages", "POST", true, &p,
          nullptr, [t](uint16_t http_code, bool success) {});
  }
}
void DiscordClient::itemSelected(std::string type, const SearchItem* item) {
  if (type == "guild") {
    DiscordGuildPayload* p =
        reinterpret_cast<DiscordGuildPayload*>(item->user_data);
    if (p) {
      active_guild = p;
      DiscordGuildPayload& intern = guilds[p->id];
      channel_items.clear();
      for (std::map<std::string, DiscordChannelPayload>::iterator it =
               intern.channels.begin();
           it != intern.channels.end(); ++it) {
        DiscordChannelPayload& ch = it->second;
        if (ch.type == Category)
          continue;
        SearchItem item;
        item.user_data = &ch;
        std::string name = ch.type == VcChannel ? "V: " : "T: ";
        name += ch.name;
        item.name = name;
        channel_items.push_back(item);
      }
      components->channel_list.setItems(&channel_items);
      AppState::gState->setTextReceiver(&components->channel_list);
      components->root_list_display = 3;
    }
  } else if (type == "dm") {
    loadChannel((DiscordChannelPayload*)item->user_data);
    AppState::gState->setTextReceiver(&components->chat_input);
  } else if (type == "channel") {
    DiscordChannelPayload* ch = (DiscordChannelPayload*)item->user_data;
    if (ch->type == VcChannel) {
      std::string gId = active_guild->id;
      initVc(gId, ch->id);
      return;
    }
    loadChannel(ch);
    AppState::gState->setTextReceiver(&components->chat_input);
  }
}
std::string DiscordClient::computeVcName() {
  std::string connected =
      "[" + std::to_string(vcClient.connectedIds.size() + 1) + "]";
  std::string ch_id = vcClient.channel_id;
  if (private_channels.count(ch_id)) {
    std::string ch_name = getChannelName(private_channels[ch_id]);
    return "VC" + connected + ch_name;
  } else {
    std::string guild_id = vcClient.guild_id;
    if (guilds.count(guild_id)) {
      DiscordGuildPayload& g = guilds[guild_id];
      if (g.channels.count(ch_id)) {
        DiscordChannelPayload& ch = g.channels[ch_id];
        return "VC" + connected + g.name + ":" + ch.name;
      }
    } else {
      return "VC" + connected + "Unkown Guild";
    }
  }
  return "";
}
void DiscordClient::renderUserInfo() {
  auto mlist = components->message_list;
  std::string targetId = "";
  if (mlist.selected_index != -1) {
    auto index = mlist.messages.size() - mlist.selected_index - 1;
    RenderMessage* m = mlist.messages[index];
    DiscordMessagePayload& msg = m->m_holder->message;
    targetId = msg.author.id;
  } else if (active_channel_ptr) {
    if (active_channel_ptr->type == DmText) {
      targetId = active_channel_ptr->recipient_ids[0];
    }
  }
  if (targetId.length()) {
    DiscordRichUser* user = new DiscordRichUser();
    auto* t = this;
    request("/users/" + targetId + "/profile?with_mutual_guilds=true", "GET",
            false, nullptr, user, [t, user](uint16_t http_code, bool success) {
              if (!success) {
                return;
              }
              t->components->runLater(new std::function([t, user]() {
                DiscordPresence pr;
                if(t->presences.count(user->user.id)) {
                  pr = t->presences[user->user.id];
                }
                t->components->userInfo.initFrom(*user, pr);
                t->components->setActivePopover(&t->components->userInfo);
                delete user;
              }));
            });
  }
}
void DiscordClient::onMessage(DiscordBaseMessage& msg) {
  if (state == Handshaking && msg.opCode == DiscordOpCode::AuthResponse) {
    DiscordInitResponseMessage initResponse;
    state = Connected;
    initResponse.fromJson(msg.data);
    hb_interval = initResponse.heartbeat_interval;
    std::cout << hb_interval << "\n";
    auto* t = this;
    hb_thread = new std::thread([t]() {
      DiscordHeartbeatMessage hbMsg;
      while (true) {
        t->sendMessage(Heartbeat, hbMsg);
        std::this_thread::sleep_for(std::chrono::milliseconds(t->hb_interval));
      }
    });
  } else {
    if (msg.opCode == 0) {
      std::string evType = msg.eventType;
      if (evType == "READY") {
        DiscordReadyPayload p;
        p.fromJson(msg.data);
        onReadyPayload(p);
        auto* t = this;
        t->components->runLater(new std::function([t]() {
            t->components->status_text.setData("Ready");
            }));

      } else if (evType == "VOICE_SERVER_UPDATE") {
        DiscordVoiceServerUpdate update;
        update.fromJson(msg.data);
        vcClient.voiceServerData(update);
      } else if (evType == "VOICE_STATE_UPDATE") {
        DiscordVoiceConnectionUpdate update;
        update.fromJson(msg.data);
        bool res = vcClient.voiceStateData(update);
        if (res) {
          components->status_text.setData(computeVcName());
          AppState::gState->emptyEvent();
        }
      } else if (evType == "CHANNEL_UPDATE") {
        DiscordChannelPayload channel;
        channel.fromJson(msg.data);
        if (channel.type == DmText || channel.type == DmGroup) {
          if (private_channels.count(channel.id)) {
            DiscordChannelPayload* ptr = &private_channels[channel.id];
            ptr->fromJson(msg.data);
            for (auto& e : dm_items) {
              if (e.user_data == ptr) {
                e.name = getChannelName(*ptr);
              }
            }
          }
        }
        if (channel.id == active_channel)
          updateActiveChannel();
      } else if (evType == "MESSAGE_CREATE") {
        DiscordMessagePayload message;
        message.fromJson(msg.data);
        if (active_channel == message.channel_id) {
          message_state.add_message(message, *active_channel_ptr);
        } else if (private_channels.count(message.channel_id)) {
          DiscordChannelPayload& ch = private_channels[message.channel_id];
          message_state.add_message(message, ch);
        }
        if (active_channel == message.channel_id) {
            auto* t = this;
            std::string id = message.id;
            components->runLater(new std::function([t, id]() {
                t->components->message_list.addContent(
                    t->message_state.message_index[id]);
                }));
        }
      } else if (evType == "MESSAGE_UPDATE") {
        DiscordMessagePayload message;
        message.fromJson(msg.data);
        if (message_state.message_index.count(message.id)) {
          MessageHolder* h = message_state.message_index[message.id];
          h->message = message;
          if (active_channel == message.channel_id) {
            auto* t = this;
            components->runLater(new std::function([t, h]() {
              auto& l = t->components->message_list;
              l.updateEntry(h);
            }));
          }
        }
      } else if (evType == "MESSAGE_DELETE") {
        DiscordMessageDeletePayload* deleter =
            new DiscordMessageDeletePayload();
        deleter->fromJson(msg.data);
        if (active_channel == deleter->channel_id) {
          auto* t = this;
          components->runLater(new std::function([t, deleter]() {
            t->components->message_list.removeEntry(
                t->message_state.message_index[deleter->id]);
            t->message_state.remove_message(deleter->id);
            delete deleter;
          }));
        } else {
          message_state.remove_message(deleter->id);
          delete deleter;
        }
      } else if (evType == "MESSAGE_REACTION_ADD") {
        DiscordMessageReactionAddPayload add;
        add.fromJson(msg.data);
        if (add.emote_id.length() &&
            message_state.message_index.count(add.message_id)) {
          MessageHolder* holder = message_state.message_index[add.message_id];
          DiscordMessagePayload& msg = holder->message;
          if (msg.reactions.count(add.emote_id)) {
            (&msg.reactions[add.emote_id])->count++;
          } else {
            DiscordMessageReactionPayload reaction;
            reaction.count = 1;
            reaction.emote_id = add.emote_id;
            reaction.emote_name = add.emote_name;
            reaction.me = add.user_id == user.id;
            msg.reactions[reaction.emote_id] = reaction;
          }
          if (active_channel == add.channel_id)
            AppState::gState->emptyEvent();
        }
      } else if (evType == "CALL_CREATE") {
        if (startedDmCall) {
          DiscordCallRingRequest req;
          request("/channels/" + active_channel + "/call/ring", "POST", true,
                  &req, nullptr, [](uint16_t http_code, bool success) {});
          startedDmCall = false;
        }
      } else if (evType == "CHANNEL_CREATE") {
        DiscordChannelPayload p;
        p.fromJson(msg.data);
        if (p.type == DmText || p.type == DmGroup) {
          private_channels[p.id] = p;
          SearchItem item;
          item.user_data = &private_channels[p.id];
          item.name = getChannelName(p);
          dm_items.insert(dm_items.begin(), item);
          AppState::gState->emptyEvent();
        } else if (p.type == VcChannel || p.type == TextChannel) {
          (&guilds[p.guild_id])->channels[p.id] = p;
        }
      } else if (evType == "GUILD_CREATE") {
        DiscordGuildPayload guild;
        guild.fromJson(msg.data);
        guilds[guild.id] = guild;
        SearchItem item;
        item.user_data = &guilds[guild.id];
        item.name = guild.name;
        guild_items.insert(guild_items.begin(), item);
        components->guilds_list.recompute();
        AppState::gState->emptyEvent();
      } else if (evType == "GUILD_DELETE") {
        std::string id = msg.data["id"];
        if (guilds.count(id)) {
          DiscordGuildPayload& g = guilds[id];
          uint32_t index = -1;
          for (int i = 0; i < guild_items.size(); ++i) {
            if (guild_items[i].user_data == &g) {
              index = i;
              break;
            }
          }
          if (index != -1) {
            guild_items.erase(guild_items.begin() + index);
            components->guilds_list.recompute();
          }
          for (auto& ch_raw : g.channels) {
            auto& ch = ch_raw.second;

            if (active_channel == ch.id) {
              channel_items.clear();
              components->channel_list.recompute();

              active_channel = "";
              active_channel_ptr = nullptr;
              auto* t = this;
              std::string* ptr = new std::string(ch.id);
              components->runLater(new std::function([t, ptr]() {
                t->components->message_list.clearList();
                t->message_state.remove_channel(*ptr);
                delete ptr;
                t->components->header_text.setData("");
              }));

            } else {
              if (message_state.state.count(ch.id))
                message_state.remove_channel(ch.id);
            }
          }
          guilds.erase(g.id);
          AppState::gState->emptyEvent();
        }
      } else if (evType == "CHANNEL_DELETE") {
        DiscordChannelPayload ch;
        ch.fromJson(msg.data);

        if (ch.guild_id.size()) {
          auto& guild = guilds[ch.guild_id];
          DiscordChannelPayload& lChannel = guild.channels[ch.id];
          uint32_t index = -1;
          for (int i = 0; i < channel_items.size(); ++i) {
            auto& e = channel_items[i];
            if (e.user_data == &lChannel) {
              index = i;
              break;
            }
          }
          if (index != -1) {
            channel_items.erase(channel_items.begin() + index);
            components->channel_list.recompute();
          }
          guild.channels.erase(ch.id);
        } else if (ch.type == DmText || ch.type == DmGroup) {
          auto& lChannel = private_channels[ch.id];
          uint32_t index = -1;
          for (int i = 0; i < dm_items.size(); ++i) {
            auto& e = dm_items[i];
            if (e.user_data == &lChannel) {
              index = i;
              break;
            }
          }
          if (index != -1) {
            dm_items.erase(channel_items.begin() + index);
            components->dm_list.recompute();
          }
          private_channels.erase(ch.id);
        }

        if (active_channel == ch.id) {
          active_channel = "";
          active_channel_ptr = nullptr;
          auto* t = this;
          std::string* ptr = new std::string(ch.id);
          components->runLater(new std::function([t, ptr]() {
            t->components->message_list.clearList();
            t->message_state.remove_channel(*ptr);
            delete ptr;
            t->components->header_text.setData("");
          }));
        } else {
          message_state.remove_channel(ch.id);
        }
      } else if(evType == "READY_SUPPLEMENTAL") {
        DiscordSuplementalReadyPayload r;
        r.fromJson(msg.data);
        presences = r.presences;
      }
    }
  }
}
void DiscordClient::updateActiveChannel() {
  auto* t = this;
  components->runLater(new std::function([t]() {
    t->components->header_text.setData(
        t->getChannelName(*t->active_channel_ptr));
  }));
}
void DiscordClient::sendEvent(std::string name, DiscordMessage& msg) {
  if (!connecting)
    return;
  json d = msg.getJson();
  DiscordBaseMessage m = {Ev, d, name};
  json toSend = m;
  std::cout << toSend << "\n";
  m_web_socket.send(toSend.dump());
};

void DiscordClient::loadChannel(DiscordChannelPayload* channel) {
  std::string channel_id = channel->id;
  std::cout << channel_id << "\n";
  if (active_channel == channel_id)
    return;

  bool needsLoad = !message_state.state.count(channel_id) ||
                   (&message_state.state[channel_id])->loaded == false;
  std::cout << needsLoad << ":" << channel_id << "\n";
  if (needsLoad) {
    auto* t = this;
    DiscordMessageListStruct* list = new DiscordMessageListStruct();
    request("/channels/" + channel_id + "/messages?limit=50", "GET", false,
            nullptr, list,
            [list, channel, t](uint16_t http_code, bool success) {
              if (!success) {
                delete list;
                t->components->runLater(new std::function(
                    [t, channel]() { t->activateChannel(channel); }));
                return;
              }
              std::vector<DiscordMessagePayload>& messages = list->messages;
              t->message_state.load_channel(messages, *channel);

              delete list;

              t->components->runLater(new std::function(
                  [t, channel]() { t->activateChannel(channel); }));
            });
  } else {
    activateChannel(channel);
  }
}
std::string DiscordClient::getChannelName(DiscordChannelPayload& channel) {
  if (channel.name.length())
    return channel.name;
  if (channel.type == DmText) {
    if (readyP.users.count(channel.recipient_ids[0])) {
      return readyP.users[channel.recipient_ids[0]].username + "#" +
             readyP.users[channel.recipient_ids[0]].discriminator;
    } else {
      return "-";
    }
  } else if (channel.type == DmGroup) {
    std::string name = "";
    if (!channel.recipient_ids.size()) {
      return "Group: Unnamed";
    } else {
      for (auto e : channel.recipient_ids) {
        if (readyP.users.count(e)) {
          name += readyP.users[e].username + ", ";
        }
      }
    }
    name = name.substr(0, name.length() - 2);
    return "Group: " + name;
  }
  return "";
}

void DiscordClient::activateChannel(DiscordChannelPayload* ch) {
  std::string id = ch->id;
  std::cout << "active channel: " << id << "\n";
  DiscordChannelPayload& ref = *ch;
  components->header_text.setData(getChannelName(ref));
  components->message_list.clearList();
  ChannelState& state = message_state.state[id];
  for (auto* msg : state.messages) {
    components->message_list.addContent(msg);
  }
  components->message_list.scrollEnd();
  active_channel = id;
  active_channel_ptr = ch;
  if (readyP.read_states.count(id)) {
    DiscordReadState& st = readyP.read_states[id];
    std::string last_id = state.messages[state.messages.size() - 1]->id;
    if (st.last_message_id != last_id) {
      DiscordMessageAck noOp;
      request("/channels/" + id + "/messages/" + last_id + "/ack", "POST", true,
              &noOp, nullptr, [](uint16_t http_code, bool success) {});
      readyP.read_states[id].last_message_id = last_id;
    }
  }
  DiscordChannelSelectMessage sel;
  sel.channel_id = id;
  sendMessage(SelectChannel, sel);
}
void DiscordClient::sendMessage(DiscordOpCode op, DiscordMessage& msg) {
  if (!connecting)
    return;
  json d = msg.getJson();
  DiscordBaseMessage m = {op, d, ""};
  json toSend = m;
  std::cout << toSend << "\n";
  m_web_socket.send(toSend.dump());
};
void DiscordClient::request(std::string path,
                            std::string method,
                            bool hasBody,
                            DiscordMessage* body,
                            DiscordMessage* out,
                            const HttpResultCallback& callback) {
  std::cout << "http req: " << method << ":" << path << "\n";
  ix::WebSocketHttpHeaders headers;
  headers["Authorization"] = m_token;
  if (method != "GET")
    headers["Content-Type"] = "application/json";
  headers["User-Agent"] =
      "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 "
      "(KHTML, like Gecko) discord/0.0.266 Chrome/91.0.4472.164 "
      "Electron/13.6.6 Safari/537.3";
  std::string url = api_base + path;
  auto req = httpClient.createRequest(url, method);
  req->extraHeaders = headers;
  if (hasBody) {
    json b = body->getJson();
    req->body = b.dump();
  }
  httpClient.performRequest(
      req, [out, callback](const ix::HttpResponsePtr& response) {
        auto errorCode = response->errorCode;
        auto responseCode = response->statusCode;
        bool success = errorCode == ix::HttpErrorCode::Ok;
        if (success && responseCode == 200) {
          auto payload = response->body;
          if (payload.length() && out != nullptr) {
            json parsed = json::parse(payload);
            out->fromJson(parsed);
          }
        }
        callback(responseCode, success);
      });
}
void DiscordClient::request(std::string path,
                            std::string method,
                            bool hasBody,
                            DiscordMessage* body,
                            std::vector<HttpFileEntry>& files,
                            DiscordMessage* out,
                            const HttpResultCallback& callback) {
  std::cout << "http req multipart: " << method << ":" << path << "\n";
  ix::WebSocketHttpHeaders headers;
     json b = body->getJson();
  std::string jsonPayload = b.dump();
  FormData formData = FormData::generate(files, jsonPayload);
  headers["Authorization"] = m_token;
  if (method != "GET")
    headers["Content-Type"] = "multipart/form-data; boundary=----" + formData.boundary;
  headers["User-Agent"] =
      "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 "
      "(KHTML, like Gecko) discord/0.0.266 Chrome/91.0.4472.164 "
      "Electron/13.6.6 Safari/537.3";
  std::string url = api_base + path;
  auto req = httpClient.createRequest(url, method);
  req->extraHeaders = headers;
  if (hasBody) {
    req->body = formData.data;
  }
  httpClient.performRequest(
      req, [out, callback](const ix::HttpResponsePtr& response) {
        auto errorCode = response->errorCode;
        auto responseCode = response->statusCode;
        bool success = errorCode == ix::HttpErrorCode::Ok;
        if (success && responseCode == 200) {
          auto payload = response->body;
          if (payload.length() && out != nullptr) {
            json parsed = json::parse(payload);
            out->fromJson(parsed);
          }
        }
        callback(responseCode, success);
      });
}
void DiscordClient::fetchMore() {
  if (!active_channel.length() || fetching ||
      components->message_list.msg_load_ptr != nullptr) {
    return;
  }
  ChannelState& ch_state = message_state.state[active_channel];

  if (ch_state.reached_end || !ch_state.loaded)
    return;
  std::cout << "invoked fetch more\n";
  fetching = true;
  std::string first = ch_state.messages[0]->id;
  auto* t = this;
  DiscordMessageListStruct* list = new DiscordMessageListStruct();
  request(
      "/channels/" + active_channel + "/messages?limit=50&before=" + first,
      "GET", false, nullptr, list, [t, list](uint16_t http_code, bool success) {
        if (!success)
          return;
        std::vector<DiscordMessagePayload>& messages = list->messages;
        t->message_state.state[t->active_channel].reached_end =
            messages.size() < 50;
        auto out =
            t->message_state.prepend_messages(messages, *t->active_channel_ptr);
        std::vector<MessageHolder*>* ptr = new std::vector<MessageHolder*>(out);
        t->components->message_list.msg_load_ptr = ptr;
        t->fetching = false;
        delete list;
        AppState::gState->emptyEvent();
      });
}
void DiscordClient::tryDelete() {
  auto mlist = components->message_list;
  if (mlist.selected_index != -1) {
    auto index = mlist.messages.size() - mlist.selected_index - 1;
    RenderMessage* m = mlist.messages[index];
    DiscordMessagePayload& msg = m->m_holder->message;
    request("/channels/" + msg.channel_id + "/messages/" + msg.id, "DELETE",
            false, nullptr, nullptr, [](uint16_t http_code, bool success) {});
  }
}
void DiscordClient::tryEdit() {
  if(editMode)
    return;
    auto mlist = components->message_list;
  if (mlist.selected_index != -1) {
    auto index = mlist.messages.size() - mlist.selected_index - 1;
    RenderMessage* m = mlist.messages[index];
    DiscordMessagePayload& msg = m->m_holder->message;
    editMode = true;
    editingMessageId = msg.id;
    backupData = components->chat_input.text.getUtf8Value();
    components->chat_input.text.setData(msg.content);
    AppState::gState->setTextReceiver(&components->chat_input);
  }
}
void DiscordClient::cancelEdit() {
  if(!editMode)
    return;
  editMode = false;
  editingMessageId ="";
  components->chat_input.text.setData(backupData);
  backupData = "";
}
DiscordClient::DiscordClient() : httpClient(true) {}
