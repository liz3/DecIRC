#ifndef DEC_CRYPTO
#define DEC_CRYPTO
#include <mbedtls/error.h>
#include <mbedtls/pk.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/rsa.h>
#include <mbedtls/error.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#include <string>
#include <vector>
#include <map>
#include "base64.h"
#include "file_util.h"
#include "../../third-party/png/lodepng.h"

#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

enum CryptoMode {
	ModeDefault,
	ModeBoth,
	ModePublic
};
enum EncryptResult {
	Success,
	NeedPublicKey
};
#define DFL_RSA_KEYSIZE 4096

class CryptoEntry {
public:
 	void initWithNewPair();
 	void writeFile(std::string path);
 	void initFromPublicKey(std::string key);
 	void initBoth(std::string path);
 	std::string getPublicKey();
 	void encrypt(std::string content, std::vector<uint8_t>& out);
 	void decrypt(std::string ciphertext, std::vector<uint8_t>& out);

private:
	int ret;
	mbedtls_entropy_context entropy;
	std::string channel_id;
	char *p, *q;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_pk_context key;
    bool allocated = false;
    CryptoMode mode = ModeDefault;
    void baseInit(const char *pers = "gen_key");
};
class CryptoChatSet {
public:
	CryptoEntry* ownEntry = nullptr;
	CryptoEntry* remoteEntry = nullptr;
	~CryptoChatSet() {
		if(ownEntry)
			delete ownEntry;
		if(remoteEntry)
			delete remoteEntry;
	}
};
class Crypto {
private:
	std::map<std::string, CryptoChatSet*> sets;
	fs::path store_dir;
public:
	Crypto() {
      fs::path* homeDir = FileUtils::getHomeFolder();
      fs::path storeDir = ".dec_keys";
      if(!fs::exists(storeDir)) {
        fs::create_directory(storeDir);
      }
      store_dir = storeDir;

      delete homeDir;
	}
	std::string decrypt(std::string channel_id, std::vector<uint8_t> content) {
		CryptoChatSet* set = initSet(channel_id);
		if(!set->ownEntry)
			return "";
		std::vector<uint8_t> pixelData;
		unsigned width, height;

		unsigned error = lodepng::decode(pixelData, width, height, content);
		uint8_t* dataPtr = pixelData.data();
		uint32_t* intptr = reinterpret_cast<uint32_t*>(dataPtr);
		uint32_t size = intptr[0];
		uint8_t* aa = dataPtr;
		aa += 4;
		std::string raw(aa, aa+size);
		std::vector<uint8_t> out;
		set->ownEntry->decrypt(raw, out);
		return std::string(out.data(), out.data()+out.size());
	}
	std::vector<uint8_t> encrypt(std::string channel_id, std::string content, EncryptResult* status) {
		auto* set = initSet(channel_id);
		if(!set->remoteEntry) {
			*status = NeedPublicKey;
			return {};
		}
		*status = Success;
		std::vector<uint8_t> vec;
		set->remoteEntry->encrypt(content, vec);
		int pixels = vec.size() / 4;
		int rows = (pixels / 200) + 2;
		std::vector<uint8_t> pngEncodeBuffer;
		pngEncodeBuffer.resize((rows * 200) * 4);
		uint8_t* dataPtr = pngEncodeBuffer.data();
		uint32_t* intptr = reinterpret_cast<uint32_t*>(dataPtr);
		intptr[0] = vec.size();
		uint8_t* wptr = dataPtr;
		wptr += 4;
		for(uint8_t e : vec) {
			*(wptr++) = e;
		}
		std::vector<uint8_t> vecData;
        unsigned error = lodepng::encode(vecData, pngEncodeBuffer, 200, rows);
        return vecData;
	}
	void addRemoteKey(std::string ch_id, std::string key) {
		auto* set = initSet(ch_id);
		CryptoEntry* e = new CryptoEntry();
		e->initFromPublicKey(key);
		fs::path remote = store_dir / (ch_id + "1");
		if(set->remoteEntry)
			delete set->remoteEntry;
		set->remoteEntry = e;
		FileUtils::string_to_file(remote.generic_string(), key);
	}
	std::string getPublicKey(std::string ch_id) {
		auto* set = initSet(ch_id);
		return set->ownEntry->getPublicKey();
	}
	CryptoChatSet* initSet(std::string channel_id) {
		if(sets.count(channel_id))
			return sets[channel_id];
		fs::path own = store_dir / (channel_id + "0");
		fs::path remote = store_dir / (channel_id + "1");
		CryptoEntry* own_entry = new CryptoEntry();
		if(fs::exists(own)) {
			own_entry->initBoth(own.generic_string());
		} else {
			own_entry->initWithNewPair();
			own_entry->writeFile(own.generic_string());
		}
		CryptoChatSet* set = new CryptoChatSet();
		set->ownEntry = own_entry;
		if(fs::exists(remote)) {
			CryptoEntry* remote_entry = new CryptoEntry();
			remote_entry->initFromPublicKey(FileUtils::file_to_string(remote.generic_string()));
			set->remoteEntry = remote_entry;
		}
		sets[channel_id] = set;
		return set;
	}
};
#endif