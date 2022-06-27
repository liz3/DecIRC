#ifndef DEC_HTTP_UTIL
#define DEC_HTTP_UTIL
#include <string>
#include <vector>
#ifndef _WIN32
#include <unistd.h>
#endif


struct HttpFileEntry {
	std::string name;
	std::string contentType;
	std::string id;
	std::vector<uint8_t> data;
};

class FormData {
private:
	static std::string gen_random(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    
    return tmp_s;
}
public:
	std::string data;
	std::string boundary;
	static FormData generate(std::vector<HttpFileEntry> entries, std::string jsonPayload) {
		srand((unsigned)time(NULL) * getpid());     
		FormData generated;
		std::string boundary = "DecBoundary" + FormData::gen_random(15); 
		std::string data = "";
		for (int i = 0; i < entries.size(); ++i)
		{
			HttpFileEntry& e = entries[i];
			data += "------" + boundary + "\n";
			data += "Content-Disposition: form-data; name=\"files[" + e.id + "]\"; filename=\"" + e.name + "\"\n";
			data += "Content-Type: " + e.contentType + "\n\n";
			data += std::string(e.data.data(), e.data.data()+ e.data.size());
			data += "\n";

		}
		data += "------" + boundary + "\n";
		data += "Content-Disposition: form-data; name=\"payload_json\"\n\n";

		data += jsonPayload + "\n";
		data += "------" + boundary + "--";

		generated.boundary = boundary;
		generated.data = data;
		return generated;
	}
};
#endif