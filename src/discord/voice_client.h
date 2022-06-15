#ifndef DEC_VOICE_CLIENT
#define DEC_VOICE_CLIENT
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXUserAgent.h>
#include <ixwebsocket/IXWebSocket.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <json.hpp>
#include <map>
#include <string>
#include <thread>
#include "../../third-party/rtaudio/RtAudio.h"
#include "../audio/opus/opus_wrapper.h"
#include "structs.h"

#ifdef __cplusplus
extern "C" {
#endif
#define SILK_RESAMPLER_MAX_FIR_ORDER 36
#define SILK_RESAMPLER_MAX_IIR_ORDER 6
typedef struct _silk_resampler_state_struct {
  opus_int32 sIIR[SILK_RESAMPLER_MAX_IIR_ORDER]; /* this must be the first
                                                    element of this struct */
  union {
    opus_int32 i32[SILK_RESAMPLER_MAX_FIR_ORDER];
    opus_int16 i16[SILK_RESAMPLER_MAX_FIR_ORDER];
  } sFIR;
  opus_int16 delayBuf[48];
  opus_int resampler_function;
  opus_int batchSize;
  opus_int32 invRatio_Q16;
  opus_int FIR_Order;
  opus_int FIR_Fracs;
  opus_int Fs_in_kHz;
  opus_int Fs_out_kHz;
  opus_int inputDelay;
  const opus_int16* Coefs;
} silk_resampler_state_struct;
opus_int silk_resampler_init(
    silk_resampler_state_struct* S, /* I/O  Resampler state */
    opus_int32 Fs_Hz_in,  /* I    Input sampling rate (Hz)            */
    opus_int32 Fs_Hz_out, /* I    Output sampling rate (Hz)           */
    opus_int forEnc /* I    If 1: encoder; if 0: decoder                 */
);

/*!
 * Resampler: convert from one sampling rate to another
 */
opus_int silk_resampler(
    silk_resampler_state_struct* S, /* I/O  Resampler state */
    opus_int16 out[],               /* O    Output signal               */
    const opus_int16 in[],          /* I    Input signal          */
    opus_int32 inLen /* I    Number of input samples                */
);

#ifdef __cplusplus
}
#endif
constexpr auto kFrameSize = 48 * 20;
constexpr auto kNumChannels = 2;
constexpr auto kSampleRate = 48000;
constexpr auto kFrameSizeSecs = 0.02;
class DiscordVoiceClient {
 private:
  void onWsReady();
  void onWsMessage(const ix::WebSocketMessagePtr& msg);
  void connect();
  void setupSocket();
  void send_remote(std::vector<uint8_t>& data);
  void sendSpeaking(bool speaking);
  void recv_remote(uint32_t size, std::vector<uint8_t>& out);
  void receive_loop();
  void send_loop();
  uint16_t getShortBigEndian(char* arr, int offset);
  std::map<uint32_t, opus::Decoder*> decoders_map;
  std::map<std::string, uint32_t> id_ssrc_map;
  void ensureDecoder(std::string id, uint32_t ssrc);
  uint16_t encode_seq = 0;
  uint32_t encode_count = 0;
  uint32_t timestamp = 0;
  uint32_t native_hb;
  uint32_t nonce_count = 0;

  std::thread* send_thread = nullptr;
  std::thread* recv_thread = nullptr;
  void startAudioLoops();
  static void duplicate_signal(opus_int16* in_L,
                               opus_int16* in_R,
                               opus_int16* out,
                               const size_t num_samples) {
    for (size_t i = 0; i < num_samples; ++i) {
      out[i * 2] = in_L[i];
      out[i * 2 + 1] = in_R[i];
    }
  }
  uint8_t nonce[24];
  //  RtAudio dac;

 public:
  double lastVal = 0.0;
  bool speaking = false;
  uint32_t silenceCounter = 0;
  opus::Encoder encoder;
  silk_resampler_state_struct resampler_state;
  bool running = false;
  bool needsResample = false;
  int state = -1;
  DiscordVoiceClient();
  std::string guild_id;
  std::string channel_id;
  std::string v_token;
  std::string endpoint;
  std::string session_id;
  std::string d_token;
  std::string own_ip;
  std::string remote_ip;
  std::vector<uint8_t> playBackBuffer;
  std::map<std::string, bool> connectedIds;
  uint32_t playbackCount = 0;
  int remote_port = 0;
  int own_port = 0;
  uint32_t own_ssrc = 0;
  std::vector<uint8_t> secret;
  DiscordUser* user;
  std::thread* hb_thread = nullptr;
  std::thread* native_hb_thread = nullptr;
  int hb_interval;
  int meta_received = 0;
  ix::WebSocket m_web_socket;
  int seq = 0;
#ifdef _WIN32
  SOCKET sockfd;
  sockaddr_in servaddr;
#else
  int sockfd;
  struct sockaddr_in servaddr;
#endif
  void init(std::string guild_id, std::string channel_id);
  bool computeSpeaking(double value);
  void voiceServerData(DiscordVoiceServerUpdate update);
  bool voiceStateData(DiscordVoiceConnectionUpdate);
  void sendMessage(DiscordVoiceOpCode op, DiscordMessage& msg);
  void disconnect();
  void sendAudioPacket(std::vector<uint8_t>& data, uint8_t fb = 0x78);
};
#endif