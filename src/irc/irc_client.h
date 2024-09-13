#ifndef DEC_IRC_CLIENT_H
#define DEC_IRC_CLIENT_H
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <chrono>
#include "irc_socket.h"
#include "irc_messages.hpp"
#include "IncomingMessage.h"

enum UiImpact { None, Redraw, RedrawIfActiveNetwork, RedrawIfActiveChannel };

struct IrcNetworkInfo {
  std::string network_name;
  std::string given_name;
};
enum IrcChannelType { UserChannel = 0, NormalChannel = 1, InfoChannel = 2 };
class IrcClient;
struct IrcChannel {
  std::string id;
  std::string name;
  std::string topic;
  bool notify = false;
  IrcClient* client;
  IrcChannelType type;
  std::vector<IrcMessageMsg> messages;
  std::vector<std::string> users;
};
struct IrcChannelSearchEntry {
  std::string name;
  std::string topic;
  uint32_t user_count;
};
struct IrcNameSearchEntry {
  std::string name;
  std::string channel;
  std::string mode;
};
struct IrcNameSearch {
  std::vector<std::string> channels;
  std::vector<IrcNameSearchEntry> entries;
  void clear() {
    channels.clear();
    entries.clear();
  }
};
struct WhoIsEntry {
  std::string realname;
  std::string host;
  std::string nick;
  std::string username;

  std::string server_name;
  std::string server_info;

  std::string modes;
  std::vector<std::string> channels;
  bool secure_con = false;
  bool op = false;

  uint32_t logonTime = 0;
  uint32_t idle = 0;
};
class IrcClient {
  using OnIrcMessage =
      std::function<void(const IncomingMessage, IrcClient*, UiImpact)>;
  using OnIrcDisconnect =
      std::function<void(IrcClient*)>;
 public:
  enum ConnectionState {
    Idle,
    Connecting,
    Connected,
    Authenticated,
    Disconnected,
    Failed
  };
  IrcClient(std::string host);
  IrcClient(std::string host, bool useTLS);
  void setCallback(const OnIrcMessage&);
  void setErrorCallback(const OnIrcDisconnect&);
  ~IrcClient();
  void write(std::vector<std::string> arguments);
  void write(IrcMessage& msg);
  void write(std::string final_cmd);
  void connect();
  void disconnect();
  void reset();
  bool isChannelMode(char ch);
  bool isPrefix(char ch);
  IrcChannelType readChannelType(char c);
  std::string password;
  std::string nick;
  std::string username;
  std::string realname;
  std::unordered_map<std::string, bool> notifyMap;

  std::string host;
  uint16_t port = 6697;

  bool useTLS = false;
  bool verifySSL = false;
  bool autoConnect = false;
  IrcNetworkInfo networkInfo;
  std::map<std::string, IrcChannel>& getJoinedChannels() {
    return joinedChannels;
  }
  ConnectionState getState() { return state; }
  std::map<std::string, IrcChannel> joinedChannels;
  std::vector<IrcChannelSearchEntry> channelSearch;
  std::string searchQuery;
  std::map<std::string, WhoIsEntry> userInfos;
  std::string lastWhoIs;
  IrcNameSearch nameSearch;

  std::string channelModes;
  std::string prefixes;
  bool isInNamesQuery = false;

 private:
  OnIrcMessage ircMessageListener;
  OnIrcDisconnect ircErrorHandler;
  std::string recv_buff;
  void handleMessage(std::string message);
  UiImpact handleCommand(IncomingMessage&);
  ConnectionState state = ConnectionState::Idle;
  std::thread* connection_thread = nullptr;
  IrcSocket* socket = nullptr;
  bool send_ping = false;
  std::chrono::time_point<std::chrono::system_clock> last_ping;
};
#endif