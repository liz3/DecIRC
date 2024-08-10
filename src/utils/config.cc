#include "config.h"
#include "file_util.h"
#include <algorithm>
namespace fs = std::filesystem;
fs::path* DecConfig::getHomeFolder(){
#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
#else
    const char* home = getenv("HOME");
#endif
    if(home)
      return new fs::path(home);
    return nullptr;

}
std::vector<IrcClient*> DecConfig::loadClients() {
    std::vector<IrcClient*> clients;
    load();
    if(rootConfig.contains("networks")) {
        json networks = rootConfig["networks"];
        for (auto& [key, val] : networks.items())
        {
            std::string host = val["host"];
            IrcClient* client = new IrcClient(host);
            client->networkInfo.given_name = key;
            if(val.contains("port"))
                client->port = val["port"];
            client->nick = val["nick"];
            if(val.contains("username"))
                client->username = val["username"];
            if(val.contains("password"))
                client->password = val["password"];
            if(val.contains("realname"))
                client->realname = val["realname"];
            if(val.contains("ssl"))
                client->useTLS = val["ssl"];
             if(val.contains("verify-ssl"))
                client->verifySSL = val["verify-ssl"];
             if(val.contains("auto-connect"))
                client->autoConnect = val["auto-connect"];
            if(val.contains("notify")) {
                json& notifies = val["notify"];
                for(auto& [chan, v] : notifies.items()){
                    client->notifyMap[chan] = v;
                }
            }
            clients.push_back(client);
        }
    }
    return clients;
}
json DecConfig::loadCache(const std::string& network, const std::string& channel){
     fs::path* homeDir = getHomeFolder();

    if(!homeDir){
        return json::array();
    }
    std::string channel_name = channel;
    std::replace( channel_name.begin(), channel_name.end(), '#', '_');
    channel_name += ".json";
    fs::path file = (*homeDir) / ".decirc" / "message-cache" / network / channel_name;
    if(!fs::exists(file)) {
        delete homeDir;
        return json::array();
    }
    std::string content = FileUtils::file_to_string(file.generic_string());
    delete homeDir;
    return json::parse(content);
}
void DecConfig::saveCache(const std::string& network, const std::string& channel, json& list) {
    fs::path* homeDir = getHomeFolder();
    if(!homeDir){
        return;
    }
    fs::path base_folder = (*homeDir) / ".decirc";
    if(!fs::exists(base_folder)) {
        fs::create_directory(base_folder);
    }
     base_folder = (*homeDir) / ".decirc" / "message-cache";
    if(!fs::exists(base_folder)) {
        fs::create_directory(base_folder);
    }
     base_folder = (*homeDir) / ".decirc" / "message-cache" / network;
    if(!fs::exists(base_folder)) {
        fs::create_directory(base_folder);
    }
    std::string channel_name = channel;
    std::replace( channel_name.begin(), channel_name.end(), '#', '_');
    channel_name += ".json";
    fs::path file = (*homeDir) / ".decirc" / "message-cache" / network / channel_name;
    auto to_write = list.dump();
    FileUtils::string_to_file(file.generic_string(), to_write);
    delete homeDir;
}
int DecConfig::getCacheSize() {
      load();
      if(rootConfig.contains("cache_size"))
        return rootConfig["cache_size"];
    return 500;
}
void DecConfig::saveClients(std::vector<IrcClient*>& clients) {
    json cls;
    for(const auto* client : clients) {
        json entry;
        entry["host"] = client->host;
        entry["port"] = client->port;
        entry["nick"] = client->nick;
        entry["ssl"] = client->useTLS;
        entry["verify-ssl"] = client->verifySSL;
        if(client->username.length())
            entry["username"] = client->username;
         if(client->password.length())
            entry["password"] = client->password;
         if(client->realname.length())
            entry["realname"] = client->realname;
        if(client->autoConnect)
            entry["auto-connect"] = true;
        if(client->notifyMap.size()) {
            json notifies;
            for(auto chan : client->notifyMap) {
                notifies[chan.first] = chan.second;
            }
            entry["notify"] = notifies;
        }
        cls[client->networkInfo.given_name] = entry;
    }
    rootConfig["networks"] = cls;
    save();
}
void DecConfig::load() {
    if(loaded)
        return;
    fs::path* homeDir = getHomeFolder();
    if(!homeDir){
        setDefaults();
        loaded = true;
        return;
    }
    fs::path base_folder = (*homeDir) / ".decirc";
    if(!fs::exists(base_folder)) {
        delete homeDir;
        setDefaults();
        loaded = true;
        return;
    }
    fs::path file = base_folder / "config.json";
    if(!fs::exists(file)) {
        delete homeDir;
        setDefaults();
        loaded = true;
        return;
    }
    loaded = true;
    std::string content = FileUtils::file_to_string(file.generic_string());
    rootConfig = json::parse(content);
    delete homeDir;
}
void DecConfig::save() {
    fs::path* homeDir = getHomeFolder();
    if(!homeDir){
        setDefaults();
        loaded = true;
        return;
    }
    fs::path base_folder = (*homeDir) / ".decirc";
    if(!fs::exists(base_folder)) {
        fs::create_directory(base_folder);
    }
    fs::path file = base_folder / "config.json";
    std::string to_write = rootConfig.dump(1);
    FileUtils::string_to_file(file.generic_string(), to_write);
    delete homeDir;
}
void DecConfig::setDefaults() {

}
#ifdef DEC_LIZ_PNG_UPLOAD
std::string DecConfig::getPngUploadToken() {
    load();
    if(!rootConfig.contains("liz3-png-token"))
        return "";
    return rootConfig["liz3-png-token"];
}
void DecConfig::savePngUploadToken(std::string value) {
    rootConfig["liz3-png-token"] = value;
    save();
}

std::string DecConfig::getPngUploadHost() {
    load();
    if(!rootConfig.contains("liz3-png-host"))
        return "";
    return rootConfig["liz3-png-host"];
}
void DecConfig::savePngUploadHost(std::string value) {
    rootConfig["liz3-png-host"] = value;
    save();
}
#endif