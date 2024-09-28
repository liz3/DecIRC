#ifndef DEC_CONFIG_H
#define DEC_CONFIG_H
#include <string>
#include <filesystem>
#include <vector>
#include "../../third-party/json/json.hpp"
#include "../irc/irc_client.h"

namespace fs = std::filesystem;
using json = nlohmann::json;
class DecConfig {
public:
    std::vector<IrcClient*> loadClients();
    void saveClients(std::vector<IrcClient*>& clients);
    json loadCache(const std::string& network, const std::string& channel);
    void saveCache(const std::string& network, const std::string& channel, json& list);
    int getCacheSize();
    int getFontSize();
    std::string getFallbackUserName();
#ifdef DEC_LIZ_PNG_UPLOAD
    std::string getPngUploadToken();
    void savePngUploadToken(std::string value);
        std::string getPngUploadHost();
    void savePngUploadHost(std::string value);
#endif
private:
    json rootConfig;
    bool loaded = false;
    void load();
    void save();
    void setDefaults();
    fs::path* getHomeFolder();
};
#endif