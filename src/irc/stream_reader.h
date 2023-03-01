#ifndef DEC_STREAM_READER_H
#define DEC_STREAM_READER_H
class StreamReader {
 public:
  StreamReader(std::string& msg)
      : msg(msg), offset(0), length(msg.length()), remaining(msg.length()) {}
  StreamReader(const std::string& msg)
      : msg(msg), offset(0), length(msg.length()), remaining(msg.length()) {}
  std::string readUntil(char brkChar) {
    if (remaining == 0)
      return "";
    size_t count = 0;
    for (size_t i = offset; i < length; i++) {
      if (msg[i] == brkChar)
        break;
      count++;
    }
    std::string value = msg.substr(offset, count);
    offset += count;
    remaining -= count;
    return value;
  }
  void skipUntil(char brkChar, bool include) {
    if (remaining == 0)
      return;
    size_t count = 0;
    for (size_t i = offset; i < length; i++) {
      if (msg[i] == brkChar) {
        if (include)
          count++;
        break;
      }
      count++;
    }
    offset += count;
    remaining -= count;
  }
  void skip(size_t sk) {
    size_t skip = sk > remaining ? remaining : sk;
    offset += skip;
    remaining -= skip;
  }
  std::string readUntilEnd() {
    offset = length;
    remaining = 0;
     return msg.substr(offset);
  }
  bool isNext(char what) { return msg[offset] == what; }
  bool has(char search) {
    for (size_t i = offset; i < length; i++) {
      if (msg[i] == search)
        return true;
    }
    return false;
  }
  size_t rem() { return remaining; }

 private:
  const std::string& msg;
  size_t offset;
  size_t length;
  size_t remaining;
};
#endif