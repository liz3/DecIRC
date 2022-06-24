#ifndef DEC_DISCORD_STRUCTS
#define DEC_DISCORD_STRUCTS
#include <json.hpp>
#include <map>
#include <string>
#include <vector>
using json = nlohmann::json;
// client => server { op: 13, d: { channel_id: '983795887975313449' } }

enum DiscordOpCode {
  Ev = 0,
  Heartbeat = 1,
  ClientAuth = 2,
  VoiceStateUpdate = 4,
  AuthResponse = 10,
  SelectChannel = 13
};
enum DiscordVoiceOpCode {
  VoiceAuth = 0,
  VoiceHbRequest = 1,
  VoiceInitPayload = 2,
  VoiceHeartbeat = 3,
  VoiceAuthResponse = 4,
  VoiceSetSpeaking = 5,
  VoiceHeartBeatInit = 8,
  VoiceUnk2 = 12,
  VoiceUnk1 = 16
};

struct DiscordBaseMessage {
  DiscordOpCode opCode;
  json data;
  std::string eventType;
};

struct DiscordVoiceBaseMessage {
  DiscordVoiceOpCode opCode;
  json data;
};

class DiscordMessage {
 public:
  virtual json getJson() = 0;
  virtual void fromJson(json) = 0;
};
class DiscordTokenAuthMessage : public DiscordMessage {
 private:
  std::string m_token;

 public:
  DiscordTokenAuthMessage(std::string token) { m_token = token; }

  json getJson() override {
    json j;
    j["token"] = m_token;
    j["capabilities"] = 509;
    j["compress"] = false;
    json properties;
    properties["os"] = "Mac OS X";
    properties["browser"] = "Discord Client";
    properties["release_channel"] = "ptb";
    properties["client_version"] = "0.0.62";
    properties["os_version"] = "21.2.0";
    properties["os_arch"] = "arm64";
    properties["system_locale"] = "en-US";
    properties["client_build_number"] = 130731;
    properties["client_event_source"] = nullptr;
    j["properties"] = properties;
    json presence;
    presence["status"] = "online";
    presence["since"] = 0;
    presence["activities"] = json::array();
    presence["afk"] = false;
    j["presence"] = presence;
    json client_state;

    client_state["guild_hashes"] = json::object();
    client_state["highest_last_message_id"] = "0";
    client_state["read_state_version"] = 0;
    client_state["user_guild_settings_version"] = -1;
    client_state["user_settings_version"] = -1;
    j["client_state"] = client_state;
    return j;
  };
  void fromJson(json j) override {}
};
class DiscordVoiceStateUpdate : public DiscordMessage {
  /*
    d: {
guild_id: '609022153467101186',
channel_id: '609022154213949463',
self_mute: true,
self_video: false,
preferred_region: 'rotterdam'
} */
 public:
  std::string guild_id;
  std::string channel_id;
  bool self_mute = false;
  bool self_deaf = false;
  bool self_video = false;
  std::string preferred_region = "rotterdam";
  json getJson() override {
    json j;
    if (guild_id.length())
      j["guild_id"] = guild_id;
    else
      j["guild_id"] = nullptr;
    if (channel_id.length())
      j["channel_id"] = channel_id;
    else
      j["channel_id"] = nullptr;
    j["self_mute"] = self_mute;
    j["self_deaf"] = self_deaf;
    j["self_video"] = self_video;
    j["preferred_region"] = preferred_region;
    return j;
  };
  void fromJson(json j) override {}
};
class DiscordVoiceServerUpdate : public DiscordMessage {
 public:
  std::string token;
  std::string endpoint;
  std::string guild_id;
  std::string channel_id;
  json getJson() override {
    json j = 3;
    return j;
  };
  void fromJson(json j) override {
    token = j["token"];
    endpoint = j["endpoint"];
    if (j.contains("guild_id") && j["guild_id"].is_string())
      guild_id = j["guild_id"];
    else
      guild_id = "";
    if (j.contains("channel_id") && j["channel_id"].is_string())
      channel_id = j["channel_id"];
    else
      channel_id = "";
  }
};
class DiscordVoiceConnectionUpdate : public DiscordMessage {
 public:
  std::string guild_id;
  std::string channel_id;
  std::string session_id;
  std::string user_id;
  json getJson() override {
    json j;
    return j;
  };
  void fromJson(json j) override {
    if (j.contains("session_id"))
      session_id = j["session_id"];
    else
      session_id = "";
    if (j.contains("guild_id") && j["guild_id"].is_string())
      guild_id = j["guild_id"];
    else
      guild_id = "";
    if (j.contains("channel_id") && j["channel_id"].is_string())
      channel_id = j["channel_id"];
    else
      channel_id = "";
    if (j.contains("user_id") && j["user_id"].is_string())
      user_id = j["user_id"];
    else
      user_id = "";
  }
};
class DiscordUser : public DiscordMessage {
 public:
  std::string username;
  bool verified = false;
  std::string id;
  std::string discriminator;
  std::string email;
  std::string avatar;
  uint32_t flags;
  std::string bio;
  std::string banner;

  json getJson() override {
    json j;
    return j;
  };
  void fromJson(json j) override {
    username = j["username"];
    if (j.contains("verified") && j["verified"].is_boolean())
      verified = j["verified"];
    else
      verified = false;
    id = j["id"];
    discriminator = j["discriminator"];
    if (j.contains("email"))
      email = j["email"];
    else
      email = "";
    if (j.contains("flags") && j["flags"].is_number())
      flags = j["flags"];
    else if (j.contains("public_flags") && j["public_flags"].is_number())
      flags = j["public_flags"];
    if (j.contains("avatar") && j["avatar"].is_string())
      avatar = j["avatar"];
    else
      avatar = "";

    if (j.contains("bio") && j["bio"].is_string())
      bio = j["bio"];
    else
      bio = "";

    if (j.contains("banner") && j["banner"].is_string())
      banner = j["banner"];
    else
      banner = "";
  }
};
struct DiscordUserConnection {
  std::string type;
  std::string id;
  std::string name;
  bool verified;
};
class DiscordRichUser : public DiscordMessage {
 public:
  DiscordUser user;
  std::vector<DiscordUserConnection> connections;
  std::map<std::string, std::string> mutual_guilds;
  json getJson() override {
    json j;
    return j;
  }
  void fromJson(json j) override {
    user.fromJson(j["user"]);
    if (j.contains("connected_accounts") &&
        j["connected_accounts"].is_array()) {
      for (json jj : j["connected_accounts"]) {
        DiscordUserConnection con;
        con.type = jj["type"];
        con.id = jj["id"];
        con.name = jj["name"];
        con.verified = jj["verified"];
        connections.push_back(con);
      }
    }
    if (j.contains("mutual_guilds") && j["mutual_guilds"].is_array()) {
      for (json jj : j["mutual_guilds"]) {
        std::string nick;
        if (jj["nick"].is_string())
          nick = jj["nick"];
        else
          nick = "";
        mutual_guilds[jj["id"]] = nick;
      }
    }
  }
};
class DiscordVoiceAuthPayload : public DiscordMessage {
 public:
  std::string user_id;
  std::string server_id;  // guild_id;
  std::string session_id;
  bool video = true;
  std::string token;
  std::vector<json> streams;
  DiscordVoiceAuthPayload() {
    json one;
    one["type"] = "video";
    one["rid"] = "100";
    one["quality"] = 100;
    streams.push_back(one);
    json two;
    two["type"] = "video";
    two["rid"] = "50";
    two["quality"] = 50;
    streams.push_back(two);
  }
  json getJson() override {
    json j;
    if (server_id.length())
      j["server_id"] = server_id;
    else
      j["server_id"] = nullptr;
    j["session_id"] = session_id;
    j["streams"] = streams;
    j["token"] = token;
    j["user_id"] = user_id;
    j["video"] = video;
    return j;
  };
  void fromJson(json j) override {}
};
class DiscordVoiceOp12Message : public DiscordMessage {
 public:
  uint32_t audio_ssrc;
  std::vector<json> streams;
  DiscordVoiceOp12Message() {
    json one;
    one["type"] = "video";
    one["rid"] = "100";
    one["quality"] = 100;
    streams.push_back(one);
    json two;
    two["type"] = "video";
    two["rid"] = "50";
    two["quality"] = 50;
    streams.push_back(two);
  }
  json getJson() override {
    json j;
    j["streams"] = streams;
    j["audio_ssrc"] = audio_ssrc;
    j["rtx_ssrc"] = 0;
    j["video_ssrc"] = 0;
    return j;
  };
  void fromJson(json j) override {}
};
class DiscordHeartbeatMessage : public DiscordMessage {
 public:
  json getJson() override {
    json j = 3;
    return j;
  };
  void fromJson(json j) override {}
};
class DiscordVoiceInitPayload : public DiscordMessage {
 public:
  std::string ip;
  uint32_t port;
  uint32_t ssrc;
  uint32_t hb_interval;
  std::vector<std::string> modes;
  json getJson() override {
    json j = 3;
    return j;
  };
  void fromJson(json j) override {
    ip = j["ip"];
    modes = j["modes"].get<std::vector<std::string>>();
    port = j["port"];
    ssrc = j["ssrc"];
    hb_interval = j["heartbeat_interval"];
  }
};
struct Codec {
  std::string name;
  std::string type;
  int priority;
  int payload_type;
  int rtx_payload_type;
};
class DiscordVoiceSelectionMessage : public DiscordMessage {
 public:
  std::string address;
  std::vector<Codec> codecs;
  std::string mode;
  std::string protocol = "udp";
  int port;
  std::string rtc_connection_id;
  DiscordVoiceSelectionMessage() {
    codecs = {{"opus", "audio", 1000, 120, 0},
              {"H264", "video", 1000, 101, 102},
              {"VP8", "video", 2000, 103, 104},
              {"VP9", "video", 3000, 105, 106},
              {"AV1X", "video", 4000, 107, 108}};
  }
  json getJson() override {
    json j;
    std::vector<json> sCodecs;
    for (auto& c : codecs) {
      json codec;
      codec["name"] = c.name;
      codec["type"] = c.type;
      codec["priority"] = c.priority;
      codec["payload_type"] = c.payload_type;
      if (c.rtx_payload_type > 0)
        codec["rtx_payload_type"] = c.rtx_payload_type;
      sCodecs.push_back(codec);
    }
    j["address"] = address;
    j["codecs"] = sCodecs;
    json data;
    data["address"] = address;
    data["mode"] = mode;
    data["port"] = port;
    j["data"] = data;
    j["experiments"] = json::array();
    j["mode"] = mode;
    j["port"] = port;
    j["protocol"] = protocol;
    j["rtc_connection_id"] = rtc_connection_id;
    return j;
  };
  void fromJson(json j) override {}
};
class DiscordVoiceSelectionResponsePayload : public DiscordMessage {
 public:
  std::vector<uint8_t> secret;
  std::string selectedMode;
  std::string audio_codec;
  std::string video_codec;
  std::string media_session_id;
  json getJson() override {
    json j;
    return j;
  };
  void fromJson(json j) override {
    audio_codec = j["audio_codec"];
    video_codec = j["video_codec"];
    if (j.contains("media_session_id")) {
      media_session_id = j["media_session_id"];
    }
    selectedMode = j["mode"];
    secret = j["secret_key"].get<std::vector<uint8_t>>();
  }
};
class DiscordVoiceHeartbeatMessage : public DiscordMessage {
 public:
  json getJson() override {
    std::time_t t = std::time(0);
    json j = t;
    return j;
  };
  void fromJson(json j) override {}
};
class DiscordNoOp : public DiscordMessage {
 public:
  json getJson() override {
    json j = json::object();
    return j;
  };
  void fromJson(json j) override {}
};
class DiscordMessageAck : public DiscordMessage {
 public:
  json getJson() override {
    json j = json::object();
    j["token"] = nullptr;
    return j;
  };
  void fromJson(json j) override {}
};
class DiscordVoiceSpeakingMessage : public DiscordMessage {
 public:
  bool speaking = false;
  uint32_t ssrc;
  std::string user_id;

  json getJson() override {
    json j;
    j["speaking"] = speaking ? 1 : 0;
    j["delay"] = 0;
    j["ssrc"] = ssrc;
    return j;
  };
  void fromJson(json j) override {
    if (j.contains("speaking") && j["speaking"].is_boolean())
      speaking = j["speaking"];
    else
      speaking = false;
    if (j.contains("user_id") && j["user_id"].is_string())
      user_id = j["user_id"];
    else
      user_id = "";
    if (j.contains("ssrc"))
      ssrc = j["ssrc"];
    else
      ssrc = 0;
  }
};
enum ChannelType {
  TextChannel = 0,
  DmText = 1,
  VcChannel = 2,
  DmGroup = 3,
  Category = 4,
};
struct PermissionOverwrite {
  uint16_t type;
  std::string id;
  std::string allow;
  std::string deny;
};
class DiscordChannelPayload : public DiscordMessage {
 public:
  std::string name;
  std::string id;
  ChannelType type;
  std::string icon;
  uint32_t flags;
  std::string owner_id;
  std::string guild_id;
  std::vector<PermissionOverwrite> permission_overwrites;
  std::vector<std::string> recipient_ids;
  std::map<std::string, DiscordUser> recipients;
  json getJson() override {
    json j = json::object();
    return j;
  };
  void fromJson(json j) override {
    if (j.contains("name") && j["name"].is_string())
      name = j["name"];
    else
      name = "";
    id = j["id"];
    type = j["type"].get<ChannelType>();
    if (j.contains("flags"))
      flags = j["flags"];
    else
      flags = 0;
    if (j.contains("owner_id"))
      owner_id = j["owner_id"];
    else
      owner_id = "";

    if (j.contains("guild_id") && j["guild_id"].is_string())
      guild_id = j["guild_id"];
    else
      guild_id = "";

    if (j.contains("icon") && j["icon"].is_string())
      icon = j["icon"];
    else
      icon = "";

    if (j.contains("recipient_ids") && j["recipient_ids"].is_array())
      recipient_ids = j["recipient_ids"].get<std::vector<std::string>>();

    if (j.contains("recipients") && j["recipients"].is_array()) {
      for (json u : j["recipients"]) {
        DiscordUser user;
        user.fromJson(u);
        recipients[user.id] = user;
      }
    }

    if (j.contains("permission_overwrites") &&
        j["permission_overwrites"].is_array()) {
      for (json e : j["permission_overwrites"]) {
        PermissionOverwrite ow;

        ow.type = e["type"];
        ow.id = e["id"];
        ow.allow = e["allow"];
        ow.deny = e["deny"];
        permission_overwrites.push_back(ow);
      }
    }
  }
};
class DiscordRolePayload : public DiscordMessage {
 public:
  std::string name;
  std::string id;
  std::string icon;
  bool hoist;
  bool mentionable;
  bool managed;
  uint32_t position;
  json getJson() override {
    json j = json::object();
    return j;
  };
  void fromJson(json j) override {
    name = j["name"];
    id = j["id"];
    if (j.contains("icon") && j["icon"].is_string())
      icon = j["icon"];
    else
      icon = "";
    if (j.contains("hoist") && j["hoist"].is_boolean())
      hoist = j["hoist"];
    else
      hoist = false;
    if (j.contains("mentionable") && j["mentionable"].is_boolean())
      mentionable = j["mentionable"];
    else
      mentionable = false;
    if (j.contains("managed") && j["managed"].is_boolean())
      managed = j["managed"];
    else
      managed = false;
    if (j.contains("position") && j["position"].is_number())
      position = j["position"];
    else
      position = false;
  }
};
class DiscordGuildPayload : public DiscordMessage {
 public:
  std::string id;
  std::string owner_id;
  std::string name;
  uint32_t member_count;
  std::string banner;
  std::string icon;
  std::string rules_channel_id;
  std::string joined_at;
  bool nsfw;
  bool large;
  std::map<std::string, DiscordRolePayload> roles;
  std::map<std::string, DiscordChannelPayload> channels;
  json getJson() override {
    json j;
    return j;
  };
  void fromJson(json j) override {
    id = j["id"];
    if (j.contains("owner_id") && j["owner_id"].is_string())
      owner_id = j["owner_id"];
    else
      owner_id = "";
    if (j.contains("name") && j["name"].is_string())
      name = j["name"];
    else
      name = "";
    if (j.contains("member_count") && j["member_count"].is_number()) {
      member_count = j["member_count"];
    }
    if (j.contains("banner") && j["banner"].is_string())
      banner = j["banner"];
    else
      banner = "";
    if (j.contains("icon") && j["icon"].is_string())
      icon = j["icon"];
    else
      icon = "";
    if (j.contains("rules_channel_id") && j["rules_channel_id"].is_string())
      rules_channel_id = j["rules_channel_id"];
    else
      rules_channel_id = "";

    if (j.contains("nsfw") && j["nsfw"].is_boolean())
      nsfw = j["nsfw"];
    else
      nsfw = false;
    if (j.contains("large") && j["large"].is_boolean())
        large = j["large"];
    else
        large = false;

    if (j.contains("roles") && j["roles"].is_array()) {
      for (json r : j["roles"]) {
        DiscordRolePayload role;
        role.fromJson(r);
        roles[role.id] = role;
      }
    }
    if (j.contains("channels") && j["channels"].is_array()) {
      for (json r : j["channels"]) {
        DiscordChannelPayload channel;
        channel.fromJson(r);
        channels[channel.id] = channel;
      }
    }
  }
};
class DiscordMessageAttachment : public DiscordMessage {
 public:
  std::string id;
  uint32_t size;
  std::string content_type;
  std::string filename;
  std::string proxy_url;
  uint32_t height, width;
  std::string url;

  json getJson() override {
    json j;
    return j;
  };
  void fromJson(json j) override {
    id = j["id"];
    if (j.contains("size") && j["size"].is_number())
      size = j["size"];
    else
      size = 0;
    if (j.contains("width") && j["width"].is_number())
      width = j["width"];
    else
      width = 0;
    if (j.contains("height") && j["height"].is_number())
      height = j["height"];
    else
      height = 0;
    if (j.contains("content_type") && j["content_type"].is_string())
      content_type = j["content_type"];
    else
      content_type = "";
    if (j.contains("filename") && j["filename"].is_string())
      filename = j["filename"];
    else
      filename = "";
    if (j.contains("proxy_url") && j["proxy_url"].is_string())
      proxy_url = j["proxy_url"];
    else
      proxy_url = "";
    if (j.contains("url") && j["url"].is_string())
      url = j["url"];
    else
      url = "";
  }
};
class DiscordMessageEmbed : public DiscordMessage {
 public:
  std::string url;
  std::string title;
  std::string description;
  uint32_t color;
  std::string image_url = "";
  uint32_t image_width = 0, image_height = 0;
  std::string footer_text;
  std::string type;
  json getJson() override {
    json j;
    return j;
  };
  void fromJson(json j) override {
    if (j.contains("url") && j["url"].is_string())
      url = j["url"];
    else
      url = "";
    if (j.contains("title") && j["title"].is_string())
      title = j["title"];
    else
      title = "";
    if (j.contains("type") && j["type"].is_string())
      type = j["type"];
    else
      type = "";
    if (j.contains("description") && j["description"].is_string())
      description = j["description"];
    else
      description = "";

    if (j.contains("color") && j["color"].is_number())
      color = j["color"];
    else
      color = 0;

    if (type == "image" || type == "article") {
      if (j.contains("thumbnail") && j["thumbnail"].is_object()) {
        json img = j["thumbnail"];
        if (img.contains("proxy_url") && img["proxy_url"].is_string()) {
          image_url = img["proxy_url"];
          image_width = img["width"];
          image_height = img["height"];
        }
      }
    } else if (j.contains("image") && j["image"].is_object()) {
      json img = j["image"];
      if (img.contains("proxy_url") && img["proxy_url"].is_string()) {
        image_url = img["proxy_url"];
        image_width = img["width"];
        image_height = img["height"];
      }
    }

    if (j.contains("footer") && j["footer"].is_object()) {
      footer_text = j["footer"]["text"];
    } else {
      footer_text = "";
    }
  }
};
class DiscordMessageReactionPayload : public DiscordMessage {
 public:
  std::string emote_id;
  bool me = false;
  std::string emote_name;
  std::string utf_name;
  uint32_t count;
  json getJson() override {
    json j;

    return j;
  };
  void fromJson(json j) override {
    count = j["count"];
    me = j["me"];
    json emote = j["emoji"];
    if (emote.contains("id") && emote["id"].is_string()) {
        emote_id = emote["id"];
        if (emote.contains("name") && emote["name"].is_string())
            emote_name = emote["name"];
        else
            emote_name = "";
    }
    else {
        utf_name = emote["name"];
        emote_name = "";
        emote_id = "";
    }

  }
};
class DiscordMessageReactionAddPayload : public DiscordMessage {
 public:
  std::string emote_id;
  std::string emote_name;
  std::string user_id;
  std::string channel_id;
  std::string message_id;
  json getJson() override {
    json j;

    return j;
  };
  void fromJson(json j) override {
    json emote = j["emoji"];
    if (emote.contains("id") && emote["id"].is_string())
      emote_id = emote["id"];
    else
      emote_id = "";
    emote_name = emote["name"];
    user_id = j["user_id"];
    channel_id = j["channel_id"];
    message_id = j["message_id"];
  }
};
class DiscordMessagePayload : public DiscordMessage {
 public:
  DiscordUser author;
  std::string nonce;
  std::string id;
  std::string content;
  std::string channel_id;
  int32_t type;
  std::string edited_timestamp;
  std::map<std::string, DiscordUser> mentions;
  std::map<std::string, DiscordMessageAttachment> attachments;
  std::vector<DiscordMessageEmbed> embeds;
  std::map<std::string, DiscordMessageReactionPayload> reactions;
  json getJson() override {
    json j;
    if (nonce.length())
      j["nonce"] = nonce;
    if (content.length())
      j["content"] = content;
    j["tts"] = false;
    return j;
  };
  void fromJson(json j) override {
    id = j["id"];
    nonce = "";
    if (j.contains("author"))
      author.fromJson(j["author"]);
    if (j.contains("nonce") && j["nonce"].is_string())
      nonce = j["nonce"];
    else
      nonce = "";
    if (j.contains("type") && j["type"].is_number())
      type = j["type"];
    else
      type = -1;
    if (j.contains("content") && j["content"].is_string())
      content = j["content"];
    else
      content = "";
    if (j.contains("channel_id") && j["channel_id"].is_string())
      channel_id = j["channel_id"];
    else
      channel_id = "";

    if(j.contains("edited_timestamp") && j["edited_timestamp"].is_string())
      edited_timestamp = j["edited_timestamp"];
    else
      edited_timestamp = "";

    if (j.contains("mentions") && j["mentions"].is_array()) {
      for (json u : j["mentions"]) {
        DiscordUser user;
        user.fromJson(u);
        mentions[user.id] = user;
      }
    }
    if (j.contains("attachments") && j["attachments"].is_array()) {
      for (json u : j["attachments"]) {
        DiscordMessageAttachment at;
        at.fromJson(u);
        attachments[at.id] = at;
      }
    }
    if (j.contains("embeds") && j["embeds"].is_array()) {
      for (json u : j["embeds"]) {
        DiscordMessageEmbed at;
        at.fromJson(u);
        embeds.push_back(at);
      }
    }
    if (j.contains("reactions") && j["reactions"].is_array()) {
      for (json u : j["reactions"]) {
        DiscordMessageReactionPayload at;
        at.fromJson(u);
        reactions[at.emote_id] = at;
      }
    }
  }
};
class DiscordReadState : public DiscordMessage {
 public:
  std::string id;
  std::string last_message_id;
  uint32_t mention_count;
  json getJson() override {
    json j;

    return j;
  };
  void fromJson(json j) override {
    id = j["id"];
    if (j["last_message_id"].is_string())
      last_message_id = j["last_message_id"];
    else if (j["last_message_id"].is_number())
      last_message_id = std::to_string((uint64_t)j["last_message_id"]);
    if (j.contains("mention_count") && j["mention_count"].is_number())
      mention_count = j["mention_count"];
    else
      mention_count = 0;
  }
};
class DiscordCallRingRequest : public DiscordMessage {
 public:
  json getJson() override {
    json j;
    j["recipients"] = nullptr;
    return j;
  };
  void fromJson(json j) override {}
};
class DiscordReadyPayload : public DiscordMessage {
 public:
  DiscordUser user;
  std::map<std::string, DiscordUser> users;
  std::string countryCode;
  std::map<std::string, DiscordGuildPayload> guilds;
  std::map<std::string, DiscordChannelPayload> private_channels;
  std::map<std::string, DiscordReadState> read_states;

  int v = 0;
  json getJson() override {
    json j;
    return j;
  };
  void fromJson(json j) override {
    v = j["v"];
    user.fromJson(j["user"]);
    countryCode = j["country_code"];
    if (j.contains("users") && j["users"].is_array()) {
      for (json g : j["users"]) {
        DiscordUser user;
        user.fromJson(g);
        users[user.id] = user;
      }
    }
    if (j.contains("guilds") && j["guilds"].is_array()) {
      for (json g : j["guilds"]) {
        DiscordGuildPayload guild;
        guild.fromJson(g);
        guilds[guild.id] = guild;
      }
    }
    if (j.contains("private_channels") && j["private_channels"].is_array()) {
      for (json g : j["private_channels"]) {
        DiscordChannelPayload ch;
        ch.fromJson(g);
        private_channels[ch.id] = ch;
      }
    }
    if (j.contains("read_state")) {
      json read_state = j["read_state"];
      if (read_state.contains("entries") && read_state["entries"].is_array()) {
        for (json r : read_state["entries"]) {
          DiscordReadState st;
          st.fromJson(r);
          read_states[st.id] = st;
        }
      }
    }
  }
};
class DiscordChannelSelectMessage : public DiscordMessage {
 public:
  std::string channel_id;
  json getJson() override {
    json j;
    j["channel_id"] = channel_id;
    return j;
  };
  void fromJson(json j) override {}
};
class DiscordMessageDeletePayload : public DiscordMessage {
 public:
  std::string channel_id;
  std::string id;
  std::string guild_id;
  json getJson() override {
    json j;
    return j;
  };
  void fromJson(json j) override {
    id = j["id"];
    channel_id = j["channel_id"];
    if (j.contains("guild_id") && j["guild_id"].is_string())
      guild_id = j["guild_id"];
    else
      guild_id = "";
  }
};
class DiscordMessageListStruct : public DiscordMessage {
 public:
  std::vector<DiscordMessagePayload> messages;
  json getJson() override {
    json j;
    return j;
  };
  void fromJson(json j) override {
    if (!j.is_array()) {
      // wtf ?
      return;
    }
    for (auto e : j) {
      DiscordMessagePayload msg;
      msg.fromJson(e);
      messages.push_back(msg);
    }
  }
};
class DiscordInitResponseMessage : public DiscordMessage {
 public:
  uint32_t heartbeat_interval;
  std::vector<std::string> traces;
  json getJson() override {
    json j;

    return j;
  };
  void fromJson(json j) override {
    heartbeat_interval = j["heartbeat_interval"];
    if (j.contains("_trace")) {
      traces = j["_trace"].get<std::vector<std::string>>();
    }
  }
};
class DiscordPresence : public DiscordMessage {
 public:
  std::string user_id;
  std::string status;
  std::string custom_status = "";
  std::string emote_id = "";
  std::string utf_emote = "";
  json getJson() override {
    json j;
    return j;
  };
  void fromJson(json j) override {
    user_id = j["user_id"];
    if(j.contains("status") && j["status"].is_string())
      status = j["status"];
    else
      status = "";
    if(j.contains("activities") && j["activities"].is_array()) {
      for(json activity : j["activities"]) {
          if(activity.contains("type") && activity["type"].is_number() && activity["type"] == 4) {
            if(activity.contains("state") && activity["state"].is_string()) {
              custom_status = activity["state"];
            }
            if(activity.contains("emoji") && activity["emoji"].is_object()) {
                json emoji = activity["emoji"];
                if(emoji.contains("id") && emoji["id"].is_string()) {
                  emote_id = emoji["id"];
                } else if (emoji.contains("name") && emoji["name"].is_string()) {
                  utf_emote = emoji["name"];
                }
            } 
            break;
          }
      }
    }

  }
};
class DiscordSuplementalReadyPayload : public DiscordMessage {
 public:
  std::map<std::string, DiscordPresence> presences;
  json getJson() override {
    json j;
    return j;
  };
  void fromJson(json j) override {

    if(j.contains("merged_presences") && j["merged_presences"].is_object()) {
      json merged_presences = j["merged_presences"];
      if(merged_presences.contains("guilds") && merged_presences["guilds"].is_array()) {
        for(json entryParent : merged_presences["guilds"]) {
            for(json entry : entryParent) {
                     DiscordPresence presence;
          presence.fromJson(entry);
          presences[presence.user_id] = presence;
            }
        }
      }
      if(merged_presences.contains("friends") && merged_presences["friends"].is_array()) {
        for(json entry : merged_presences["friends"]) {
          DiscordPresence presence;
          presence.fromJson(entry);
          presences[presence.user_id] = presence;
        }
      }
    }
  }
};
#endif