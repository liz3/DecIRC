#ifndef DEC_MESSAGE_UTIL
#define DEC_MESSAGE_UTIL
#include <string>
class IrcMessageUtil{
public:
    static std::string stripMessage(const std::string& input);
};
#endif