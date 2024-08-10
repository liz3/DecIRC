#include "irc_event_handler.h"
#include "../gui_components.h"
#include "../utils/notifications.h"
#include "irc_client.h"
#include "message_util.h"

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
  rawBufferChannel.id = "-1";
  rawBufferChannel.name = "--RAW BUFFER--";
  rawBufferChannel.type = NormalChannel;

  t->components->runLater(new std::function(
      [t]() { t->components->status_text.setData("Ready"); }));
  connecting = true;
  auto loadedNetworks = AppState::gState->config.loadClients();
  for (auto* n : loadedNetworks)
    addNetwork(n);
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
  if(client->autoConnect) {
  addChannel(client, client->networkInfo.given_name);
  client->connect();
  for (auto& entry : network_items) {
    if (reinterpret_cast<IrcClient*>(entry.user_data) == client) {
      entry.name = "..." + client->networkInfo.given_name;
      break;
    }
  }
}
}
void IrcEventHandler::switchRawMode() {
  rawMode = !rawMode;
  if (rawMode) {
    rawBufferChannel.client = active_network;
    loadChannel(&rawBufferChannel);

  } else {
    activateChannel(nullptr);
  }
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
  if (msg.isNumericCommand &&
      (msg.numericCommand == 401 || msg.numericCommand == 263))
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
  AppState::gState->config.saveClients(active_networks);
}
bool IrcEventHandler::isPrefixChar(char ch) {
  return ch == '@' || ch == '~' || ch == '&' || ch == '+' || ch == '%';
}
void IrcEventHandler::processMessage(const IncomingMessage& msg,
                                     IrcClient* client) {
  auto& joinedChannels = client->joinedChannels;
  if (rawMode) {
    IrcMessageMsg chatMessage(msg.command, msg.isNumericCommand);
    chatMessage.source = msg.source;
    chatMessage.content = msg.command + " " + msg.parameters;
    auto& channelMessages = rawBufferChannel.messages;
    channelMessages.push_back(chatMessage);
    auto* holder = message_state.add_message(chatMessage, rawBufferChannel);
    components->runLater(new std::function([this, holder]() {
      this->components->message_list.addContent(holder);
    }));
  }
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
    if (msg.numericCommand == 5) {
      StreamReader reader(msg.parameters);
      std::string uname = reader.readUntil(' ');
      reader.skip(1);
      while (reader.rem() > 0) {
        std::string key = reader.readUntil('=');
        reader.skip(1);
        std::string value = reader.readUntil(' ');
        reader.skip(1);
        if (key == "PREFIX") {
          StreamReader modesReader(value);
          if (!modesReader.isNext('(')) {
            continue;  // ????
          }
          modesReader.skip(1);
          client->channelModes = modesReader.readUntil(')');
          modesReader.skip(1);
          client->prefixes = modesReader.readUntilEnd();
        }
      }
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
    if (msg.numericCommand == 321) {
      client->channelSearch.clear();
    }
    if (msg.numericCommand == 322) {
      StreamReader reader(msg.parameters);
      std::string uname = reader.readUntil(' ');
      reader.skip(1);
      IrcChannelSearchEntry entry;
      entry.name = reader.readUntil(' ');
      reader.skip(1);
      std::string userCount = reader.readUntil(' ');
      entry.user_count = std::stoi(userCount);
      reader.skipUntil(':', true);
      entry.topic = reader.readUntilEnd();
      client->channelSearch.push_back(entry);
    }
    if (msg.numericCommand == 323) {
      components->channels_popover.initFrom(client, QueryPopulateType::List);
      components->runLater(new std::function([this]() {
        this->components->setActivePopover(&components->channels_popover);
      }));
    }
    if (msg.numericCommand == 353 && client->isInNamesQuery) {
      IrcNameSearch& nameSearch = client->nameSearch;
      StreamReader reader(msg.parameters);
      std::string ccl = reader.readUntil(' ');
      reader.skip(1);
      std::string mode = reader.readUntil(' ');
      reader.skip(1);
      std::string channelName = reader.readUntil(' ');
      if (!std::count(nameSearch.channels.begin(), nameSearch.channels.end(),
                      channelName))
        nameSearch.channels.push_back(channelName);
      reader.skipUntil(':', true);
      while (reader.rem() > 0) {
        std::string total = reader.readUntil(' ');
        std::string modes = "";
        while (client->isPrefix(total[0])) {
          modes += total[0];
          total = total.substr(1);
        }
        IrcNameSearchEntry entry;
        entry.channel = channelName;
        entry.mode = modes;
        entry.name = total;
        nameSearch.entries.push_back(entry);
        reader.skip(1);
      }
    }
    if (msg.numericCommand == 366 && client->isInNamesQuery) {
      components->channels_popover.initFrom(client, QueryPopulateType::Names);
      components->runLater(new std::function([this]() {
        this->components->setActivePopover(&components->channels_popover);
      }));
      client->isInNamesQuery = false;
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
    if (ch.type == IrcChannelType::UserChannel || ch.notify) {
      if (!AppState::gState->focused || active_channel_ptr != &ch) {
        std::string header =
            ch.type == IrcChannelType::UserChannel
                ? (ch.name + "@" + client->networkInfo.given_name)
                : (chatMessage.source.getName() + " in " + ch.name + "@" +
                   client->networkInfo.given_name);
        Notifications::sendNotification(header, IrcMessageUtil::stripMessage(chatMessage.content));
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
  if(client->notifyMap.count(name))
    channel.notify = client->notifyMap[name];
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
      if(ch.notify) {
        item.name = "*" + ch.name;
      }
      channel_items.push_back(item);
    }
  }

  components->channel_list.setItems(&channel_items);
}

void IrcEventHandler::sendChannelMessage(std::string content) {
#ifdef DEC_LIZ_PNG_UPLOAD
  if (sendFiles.size())
    uploadPngFile();
#endif
  if (content.length() == 0)
    return;
  StreamReader reader(content);
  if (reader.isNext('/')) {
    reader.skip(1);
    std::string command = reader.readUntil(' ');
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);
    reader.skip(1);
    if (command == "RAW")
      switchRawMode();
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
        if (key == "auto-connect") {
          client->autoConnect = value == "true";
        }
        if (key == "verify-ssl") {
          client->verifySSL = value == "true";
        }
      }
      addNetwork(client);
      AppState::gState->config.saveClients(active_networks);

    } else if (command == "DELSERVER") {
      removeNetwork(active_network);
    } else if (command == "NOTIFY") {
      if (active_channel_ptr) {
        active_channel_ptr->notify = !active_channel_ptr->notify;
        active_network->notifyMap[active_channel_ptr->name] = active_channel_ptr->notify;
        AppState::gState->config.saveClients(active_networks);
         populateChannels(active_network); 
      }
    }
    if (active_network) {
      if (command == "DISCONNECT" || command == "QUIT") {
        disconnect();
      }
      if (command == "JOIN" || command == "PART" || command == "WHOIS" ||
          command == "LIST" || command == "NAMES") {
        std::string channels = reader.readUntilEnd();
        if (command == "LIST")
          active_network->searchQuery = channels;
        if (command == "NAMES") {
          active_network->isInNamesQuery = true;
          active_network->nameSearch.clear();
        }
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
    if (rawMode) {
      active_network->write(content);
    } else {
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
  }

  components->chat_input.text.setData("");
}
void IrcEventHandler::loadChannel(IrcChannel* ch) {
  if (active_network == ch->client && active_channel_ptr == ch) {
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
void IrcEventHandler::query(IrcClient* cl, std::string& name) {
  auto& joinedChannels = cl->joinedChannels;
  if (joinedChannels.count(name))
    return;
  addChannel(cl, name);
}
void IrcEventHandler::query(std::string& name) {
  if (active_network)
    query(active_network, name);
}
void IrcEventHandler::updateActiveChannel() {}

void IrcEventHandler::activateChannel(IrcChannel* ch) {
  if (ch == nullptr) {
    components->header_text.setData("");
    components->caption_text.setData("");
    components->message_list.clearList();
    active_channel = "";
    active_channel_ptr = nullptr;
    return;
  }
  std::string id = ch->id;
  std::cout << "active channel: " << ch->name << "\n";
  IrcChannel& ref = *ch;
  components->header_text.setData(ref.name);
  components->caption_text.setData(ref.topic);
  components->message_list.clearList();
  ChannelState& state = message_state.state[id];
  for (auto* msg : state.messages) {
    components->message_list.addContent(msg);
  }
  components->message_list.scrollEnd();
  active_channel = ref.name;
  active_channel_ptr = ch;
}
void IrcEventHandler::fetchMore() {}
void IrcEventHandler::tryDelete() {}
void IrcEventHandler::tryEdit() {}
void IrcEventHandler::cancelEdit() {
  if (!editMode)
    return;
  editMode = false;
  editingMessageId = "";
  components->chat_input.text.setData(backupData);
  backupData = "";
}
IrcEventHandler::IrcEventHandler() : httpClient(true) {}
#ifdef DEC_LIZ_PNG_UPLOAD
void IrcEventHandler::uploadPngFile() {
  std::vector<HttpFileEntry> files;
  int c = 0;
  for (auto* file : sendFiles) {
    HttpFileEntry e = *file;
    e.id = std::to_string(c);
    e.fieldname = "datafile";
    files.push_back(e);
    c++;
    delete file;
  }
  FormData formData = FormData::generate(files, "");
  ix::WebSocketHttpHeaders headers;
  DecConfig& config = AppState::gState->config;
  headers["Authorization"] = config.getPngUploadToken();
      headers["Content-Type"] = "multipart/form-data; boundary=----" + formData.boundary;
  std::string url = "https://" + config.getPngUploadHost() + "/add/0";
  auto req = httpClient.createRequest(url, "POST");
  req->body = formData.data;
  req->extraHeaders = headers;
  components->chat_input.images.clear();
  sendFiles.clear();
  httpClient.performRequest(req, [this](const ix::HttpResponsePtr& response) {
    auto errorCode = response->errorCode;
    auto responseCode = response->statusCode;
    bool success = errorCode == ix::HttpErrorCode::Ok;
    if (success && responseCode == 200) {
      auto payload = response->body;
      if (payload.length() && active_network) {
        DecConfig& config = AppState::gState->config;
        IrcMessageMsg msg("PRIVMSG");
        msg.content = "https://" + config.getPngUploadHost() + "/d/" + payload;
        msg.channel = active_channel_ptr->name;
        msg.source.nick = active_network->nick;
        msg.source.onlyHost = false;
        IrcChannel& ch = active_network->joinedChannels[msg.channel];
        auto& channelMessages = ch.messages;
        channelMessages.push_back(msg);
        auto* holder = message_state.add_message(msg, ch);
          components->runLater(
        new std::function([holder, this]() {  components->message_list.addContent(holder); }));
       
        active_network->write(msg);
      }
    }
  });
}
#endif