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
#include "structs.h"
#include "voice_client.h"

class GuiComponents;
using HttpResultCallback =
    std::function<void(uint16_t http_code, bool success)>;
enum DiscordConnectionState { Handshake, Handshaking, Connected, Disconnected };
const std::string api_base = "https://discord.com/api/v9";
class DiscordClient {
 public:
  bool editMode = false;
  std::string m_token;
  ix::WebSocket m_web_socket;
  void init(GuiComponents* components);
  void sendMessage(DiscordOpCode op, DiscordMessage& msg);
  void sendEvent(std::string ev, DiscordMessage& msg);
  void initVc(std::string guild_id, std::string channel_id);
  DiscordVoiceClient vcClient;
  DiscordUser user;
  DiscordReadyPayload readyP;
  std::map<std::string, DiscordChannelPayload> private_channels;
  std::map<std::string, DiscordGuildPayload> guilds;
  std::map<std::string, DiscordPresence> presences;
  std::vector<SearchItem> dm_items;
  std::vector<SearchItem> guild_items;
  std::vector<SearchItem> channel_items;
  void itemSelected(std::string type, const SearchItem* item);
  MessageState message_state;
  DiscordClient();
  void loadChannel(DiscordChannelPayload* channel);
  std::string active_channel;
  DiscordChannelPayload* active_channel_ptr;
  std::string getChannelName(DiscordChannelPayload& channel);
  void sendChannelMessage(std::string content);
  ImageCache image_cache;
  void fetchMore();
  void tryDelete();
  bool fetching = false;
  void startDmCall();
  std::string computeVcName();
  void renderUserInfo();
  void tryEdit();
  void cancelEdit();

 private:
  DiscordGuildPayload* active_guild = nullptr;
  DiscordChannelPayload* next_active = nullptr;
  ix::HttpClient httpClient;
  bool ready = false;
  bool zInit = false;
  bool startedDmCall = false;
  std::string editingMessageId = "";
  std::string backupData = "";
  z_stream infstream;
  std::string messageBuffer;
  bool connecting = false;
  std::thread* hb_thread = nullptr;
  DiscordConnectionState state = Handshake;
  uint32_t hb_interval;
  void onWsReady();
  void onWsMessage(const ix::WebSocketMessagePtr& msg);
  void onMessage(DiscordBaseMessage& msg);
  void onReadyPayload(DiscordReadyPayload p);
  void request(std::string path,
               std::string method,
               bool hasBody,
               DiscordMessage* body,
               DiscordMessage* out,
               const HttpResultCallback& callback);
  GuiComponents* components;
  void activateChannel(DiscordChannelPayload* channel);
  void updateActiveChannel();
};

DiscordClient* create_discord_client(std::string token);

#endif