#ifndef DEC_DISCORD_CLIENT
#define DEC_DISCORD_CLIENT
#include <ixwebsocket/IXHttpClient.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXUserAgent.h>
#include <ixwebsocket/IXWebSocket.h>
#include <zlib.h>
#include <chrono>
#include <iostream>
#include <json.hpp>
#include <string>
#include <thread>
#include "../components/search_list.h"

#include "image_cache.h"
#include "message_state.h"
#include "../utils/http_util.h"
#include "irc_client.h"
class GuiComponents;
using HttpResultCallback =
    std::function<void(uint16_t http_code, bool success)>;
class IrcEventHandler {
 public:
  bool editMode = false;
  std::vector<SearchItem> dm_items;
  std::vector<SearchItem> network_items;
  std::vector<SearchItem> channel_items;
  MessageState message_state;
  std::string active_channel;
  IrcChannel* active_channel_ptr = nullptr;
  ImageCache image_cache;
  bool fetching = false;
  std::vector<HttpFileEntry*> sendFiles;
  std::vector<IrcClient*> active_networks;
  IrcClient* active_network = nullptr;

  IrcEventHandler();
  //  void loadChannel(DiscordChannelPayload* channel);
  void sendChannelMessage(std::string content);
  void init(GuiComponents* components);
  void itemSelected(std::string type, const SearchItem* item);
  //  void sendMessage(DiscordOpCode op, DiscordMessage& msg);
  // void sendEvent(std::string ev, DiscordMessage& msg);
  void initVc(std::string guild_id, std::string channel_id);
  void fetchMore();
  void tryDelete();
  void renderUserList();
  void tryEdit();
  void cancelEdit();
  void addNetwork(IrcClient* client);
  void addChannel(IrcClient* client, std::string name);
  bool isMessage(const IncomingMessage&);
  bool isWhoIs(const IncomingMessage&);
  void query(std::string& name);
  void query(IrcClient* cl, std::string& name);
  void disconnect();
  void removeFromList(std::vector<std::string>& list, const std::string& value);
  void whoIsHandler(const IncomingMessage&, IrcClient* client);
  void removeNetwork(IrcClient* client);
  void closeAll();
  bool isPrefixChar(char ch);
  void switchRawMode();
  void persistChannels();
 private:
  size_t global_channel_count = 0;
  ix::HttpClient httpClient;
  bool ready = false;
  bool zInit = false;
  bool startedDmCall = false;
  std::string editingMessageId = "";
  std::string backupData = "";
  std::string messageBuffer;
  bool connecting = false;
  std::thread* hb_thread = nullptr;
  uint32_t hb_interval;
  IrcChannel rawBufferChannel;
  bool rawMode = false;

  GuiComponents* components;
  void activateChannel(IrcChannel* channel);
  void updateActiveChannel();
  void populateChannels(IrcClient* active);
  void loadChannel(IrcChannel* ch);
  void processMessage(const IncomingMessage& msg, IrcClient* client);
  #ifdef DEC_LIZ_PNG_UPLOAD
    void uploadPngFile();
  #endif
};
IrcEventHandler* create_irc_event_handler();

#endif