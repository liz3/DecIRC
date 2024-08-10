#ifndef DEC_IRC_MESSAGES_HPP
#define DEC_IRC_MESSAGES_HPP
#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>
#include "stream_reader.h"
#include "IncomingMessage.h"
#ifdef __linux__
#include <ctime>
#include <cstring>
#endif
#include <time.h>
#include "../../third-party/json/json.hpp"
#include "../utils/base64.hpp"
using json = nlohmann::json;

class IrcMessage {
 public:
  enum IrcMessageDirection { Receive, Send, Both };
  virtual void build(std::vector<std::string>& into) = 0;
  virtual bool fromParts(const std::string&) = 0;
  IrcMessageDirection direction = IrcMessageDirection::Receive;
  std::string command;
};

class IrcMessagePass : public IrcMessage {
 public:
  IrcMessagePass(std::string p) {
    password = p;
    command = "PASS";
    direction = IrcMessageDirection::Send;
  }
  void build(std::vector<std::string>& into) override {
    into.push_back(password);
  }
  bool fromParts(const std::string&) override { return true; }

 private:
  std::string password;
};
class IrcMessageNick : public IrcMessage {
 public:
  IrcMessageNick(std::string p) {
    nick = p;
    command = "NICK";
    direction = IrcMessageDirection::Send;
  }
  void build(std::vector<std::string>& into) override { into.push_back(nick); }
  bool fromParts(const std::string&) override { return true; }

 private:
  std::string nick;
};
class IrcMessageUser : public IrcMessage {
 public:
  IrcMessageUser(std::string username, std::string realname) {
    this->username = username;
    this->realname = realname;

    command = "USER";
    direction = IrcMessageDirection::Send;
  }
  void build(std::vector<std::string>& into) override {
    into.push_back(username);
    into.push_back(hostname.length() ? hostname : "0");
    into.push_back(servername.length() ? servername : "*");
    into.push_back(realname);
  }
  bool fromParts(const std::string&) override { return true; }
  std::string hostname;
  std::string servername;

 private:
  std::string username;
  std::string realname;
};
class IrcMessageQuit : public IrcMessage {
 public:
  IrcMessageQuit() {
    command = "QUIT";
    direction = IrcMessageDirection::Send;
  }
  void build(std::vector<std::string>& into) override {
    into.push_back(":" + reason);
  }
  bool fromParts(const std::string&) override { return true; }
  std::string reason = "Disconnecting";

 private:
};
class IrcMessageNames : public IrcMessage {
 public:
  IrcMessageNames() {
    command = "NAMES";
    direction = IrcMessageDirection::Receive;
  }
  void build(std::vector<std::string>& into) override {}
  bool fromParts(const std::string& input) override {
    StreamReader reader(input);
    if (reader.isNext(' '))
      reader.skip(1);
    client = reader.readUntil(' ');
    reader.skip(1);
    symbol = reader.readUntil(' ');
    reader.skip(1);
    channel = reader.readUntil(' ');
    reader.skipUntil(':', true);
    while (reader.rem() > 0) {
      names.push_back(reader.readUntil(' '));
      reader.skip(1);
    }
    return true;
  }
  std::string client;
  std::string channel;
  std::string symbol;
  std::vector<std::string> names;
};
class IrcMessageMsg : public IrcMessage {
 public:
  IrcMessageMsg(std::string cmd) {
    command = cmd;
    numericCode = false;
    direction = IrcMessageDirection::Both;
    setTime();
  }

  IrcMessageMsg(std::string cmd, bool numeric) {
    command = cmd;
    numericCode = numeric;
    direction = IrcMessageDirection::Both;
    setTime();
  }
  void build(std::vector<std::string>& into) override {
    into.push_back(channel);
    if (action) {
      std::string msg = ":";
      msg += (char)1;
      msg += "ACTION ";
      msg += content;
      msg += (char)1;
      into.push_back(msg);
    } else
      into.push_back(":" + content);
  }
  std::string getTimeFormatted() {
    if(timeAsFormatted)
      return formattedTime;
     char buffer [80];
       strftime (buffer,sizeof buffer,"%R",&time_storage);
    return std::string(buffer, strlen(buffer));
  }
  void fromJson(json& in){
    formattedTime = in["time"];
    timeAsFormatted = true;
    channel = in["channel"];
    action = in["action"];
    std::string x = in["content"];
    content = base64::from_base64(std::string_view(x));
    source.nick = in["source"]["nick"];
    source.username = in["source"]["username"];
    source.host = in["source"]["host"];
    source.onlyHost = in["source"]["onlyHost"];
  }
  json toJson() {
    json j;
    j["time"] = getTimeFormatted();
    j["channel"] = channel;
    j["action"] = action;
    j["content"] = base64::to_base64(std::string_view(content));
    json s;
    s["nick"] = source.nick;
    s["username"] = source.username;
    s["host"] = source.host;
    s["onlyHost"] = source.onlyHost;
    j["source"] = s;

    return j;
  }
  bool fromParts(const std::string& input) override {
    StreamReader reader(input);
    if (reader.isNext(' '))
      reader.skip(1);
    if (numericCode) {
      reader.skipUntil(' ', true);
      type = 1;
    }
    channel = reader.readUntil(' ');
    std::cout << "chan name: " << channel << "\n";
    reader.skipUntil(':', true);
    if (reader.isNext((char)1)) {
      reader.skip(1);
      action = true;
    }
    std::string rawContent =
        action ? reader.readUntil((char)1) : reader.readUntilEnd();

    if (action && rawContent.find("ACTION ") == 0) {
      content = rawContent.substr(7);
    } else {
      content = rawContent;
    }
    if (numericCode) {
      content = "ERR [" + command + "] " + content;
    }
    return true;
  }
   std::time_t rawtime;
    std::tm time_storage;
  size_t type = 0;
  bool numericCode;
  Source source;
  std::string channel;
  std::string content;
  bool action = false;


 private:
  bool timeAsFormatted = false;
  std::string formattedTime;
    void setTime() {
    rawtime = std::time(nullptr);
    #ifdef _WIN32
     localtime_s(&time_storage, &rawtime);
     #else
       localtime_r( &rawtime, (struct std::tm*)(&time_storage));
     #endif

    }
};
#endif