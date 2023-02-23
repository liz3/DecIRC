#include "irc_client.h"
#include <iostream>
#include "IncomingMessage.h"
#include "stream_reader.h"

IrcClient::IrcClient(std::string host)
    : host(host), ircMessageListener(OnIrcMessage()) {}
IrcClient::IrcClient(std::string host, bool useTLS)
    : host(host), useTLS(useTLS), ircMessageListener(OnIrcMessage()) {}
void IrcClient::connect() {
  if (state != IrcClient::ConnectionState::Idle)
    return;
  state = IrcClient::ConnectionState::Connecting;
  connection_thread = new std::thread([this]() {
    socket = new IrcSocket(host, port, useTLS, verifySSL);
    if (!socket->isConnected()) {
      state = IrcClient::ConnectionState::Failed;
      return;
    }
    if (password.length()) {
      IrcMessagePass passMsg(password);
      write(passMsg);
    }
    IrcMessageNick nickMsg(nick);
    write(nickMsg);
    IrcMessageUser user(username, realname);
    write(user);
    state = IrcClient::ConnectionState::Connected;
    while (state != ConnectionState::Connecting &&
           state != ConnectionState::Disconnected) {
      std::string out = socket->read();
      if (out.length() == 0)
        continue;
      recv_buff += out;
      if (recv_buff.length() > 2) {
        std::string temp = recv_buff;
        while (true) {
          auto pos = temp.find("\r\n");
          if (pos == std::string::npos) {
            break;
          }
          std::string message = temp.substr(0, pos);
          handleMessage(std::move(message));
          temp = temp.substr(pos + 2);
        }
        recv_buff = temp;
      }
    }
  });
}
void IrcClient::reset() {
  networkInfo.network_name = "";
  joinedChannels.clear();
  state = ConnectionState::Idle;
}
void IrcClient::disconnect() {
  state = ConnectionState::Disconnected;
  if (connection_thread != nullptr) {
    connection_thread->join();
    delete connection_thread;
    connection_thread = nullptr;
  }
  if (socket != nullptr) {
    socket->close();
    delete socket;
    socket = nullptr;
  }
}
void IrcClient::handleMessage(std::string raw) {
  std::cout << "<" << raw << "\n";
  if (raw.length() == 0)
    return;
  StreamReader reader(raw);
  IncomingMessage message;
  message.raw = raw;
  if (reader.isNext('@')) {
    reader.skip(1);
    message.tag = reader.readUntil(' ');
    reader.skip(1);
  }
  if (reader.isNext(':')) {
    reader.skip(1);
    std::string source = reader.readUntil(' ');
    StreamReader sourceReader(source);
    if (sourceReader.has('!')) {
      message.source.onlyHost = false;
      message.source.nick = sourceReader.readUntil('!');
      sourceReader.skip(1);
      message.source.username = sourceReader.readUntil('@');
      sourceReader.skip(1);
      message.source.host = sourceReader.readUntilEnd();
    } else {
      message.source.onlyHost = true;
      message.source.host = source;
    }
    message.forUser = source.find(nick) == 0;
    reader.skip(1);
  }
  std::string command = reader.readUntil(' ');
  message.command = command;
  {
    bool broken = false;
    for (char& c : command) {
      if (c < '0' || c > '9') {
        broken = true;
        break;
      }
    }
    message.isNumericCommand = !broken;
    if (message.isNumericCommand)
      message.numericCommand = std::stoi(message.command);
  }
  reader.skip(1);
  message.parameters = reader.readUntilEnd();

  ircMessageListener(message, this, handleCommand(message));
}
//<:Nay!jeda@hellomouse/dev/cryb.jeda PRIVMSG Liz3 :derp
UiImpact IrcClient::handleCommand(IncomingMessage& msg) {
  return UiImpact::None;
}
IrcChannelType IrcClient::readChannelType(char c) {
  if (c == '#')
    return IrcChannelType::NormalChannel;
  return IrcChannelType::UserChannel;
}
void IrcClient::setCallback(const OnIrcMessage& cb) {
  ircMessageListener = cb;
}
void IrcClient::write(std::string final_cmd) {
  std::string msg = final_cmd + "\r\n";
  socket->write(msg);
}
void IrcClient::write(IrcMessage& msg) {
  if (msg.direction != IrcMessage::IrcMessageDirection::Send &&
      msg.direction != IrcMessage::IrcMessageDirection::Both)
    return;
  std::vector<std::string> parts = {msg.command};
  msg.build(parts);
  write(parts);
}
void IrcClient::write(std::vector<std::string> parts) {
  std::string final;
  size_t remaining_args = parts.size();
  for (auto& part : parts) {
    final += part;
    remaining_args--;
    if (remaining_args > 0)
      final += " ";
  }
  write(final);
}
IrcClient::~IrcClient() {
  disconnect();
}