#include "voice_client.h"

#include <sodium.h>
#include <sstream>
#include "../AppState.h"

int audio_callback(void* outputBuffer,
                   void* /*inputBuffer*/,
                   unsigned int nBufferFrames,
                   double /*streamTime*/,
                   RtAudioStreamStatus /*status*/,
                   void* data) {
  DiscordVoiceClient* oData = (DiscordVoiceClient*)data;
  uint32_t to_read = 4 * nBufferFrames;
  if (oData->playBackBuffer.size() < to_read) {
    return 0;
  }
  std::memcpy(outputBuffer, oData->playBackBuffer.data(), to_read);
  oData->playBackBuffer.erase(oData->playBackBuffer.begin(),
                              oData->playBackBuffer.begin() + to_read);

  return 0;
}
void to_json(json& j, const DiscordVoiceBaseMessage& o) {
  j = json{{"op", (int32_t)o.opCode}, {"d", o.data}};
}

void from_json(const json& j, DiscordVoiceBaseMessage& o) {
  j.at("op").get_to(o.opCode);
  j.at("d").get_to(o.data);
}

int send_callback(void* /*outputBuffer*/,
                  void* inputBuffer,
                  unsigned int nBufferFrames,
                  double /*streamTime*/,
                  RtAudioStreamStatus /*status*/,
                  void* data) {
  DiscordVoiceClient* oData = (DiscordVoiceClient*)data;
  std::vector<opus_int16> outBuff;
  uint64_t total = 0;
  for (int i = 0; i < nBufferFrames; ++i) {
    uint16_t v = ((uint16_t*)inputBuffer)[i];
    total += v;
    outBuff.push_back(v);
    outBuff.push_back(v);
  }
  double meansquare = sqrt(
      (std::inner_product(outBuff.begin(), outBuff.end(), outBuff.begin(), 0)) /
      static_cast<double>(outBuff.size()));
  if (!oData->computeSpeaking(meansquare))
    return 0;

  std::vector<std::vector<uint8_t>> encoded;
  if (oData->needsResample) {
    opus_int16* upSampled = new opus_int16[outBuff.size() * 3];
    memset(upSampled, 0, outBuff.size() * 3);
    auto resampleRes = silk_resampler(&oData->resampler_state, upSampled,
                                      outBuff.data(), outBuff.size());
    std::vector<opus_int16> encodeBuffer(upSampled,
                                         upSampled + outBuff.size() * 3);
    encoded = oData->encoder.Encode(encodeBuffer, kFrameSize);
    delete[] upSampled;
  } else {
    std::vector<opus_int16> encodeBuffer(outBuff.data(),
                                         outBuff.data() + outBuff.size());
    encoded = oData->encoder.Encode(encodeBuffer, kFrameSize);
  }
  if (encoded.size())
    oData->sendAudioPacket(encoded[0]);

  return 0;
}

DiscordVoiceClient::DiscordVoiceClient()
    : encoder(
          opus::Encoder(kSampleRate, kNumChannels, OPUS_APPLICATION_AUDIO)) {}

bool DiscordVoiceClient::computeSpeaking(double value) {
  if (value > 100 && !speaking) {
    this->speaking = true;
    sendSpeaking(true);

    return true;
  } else if (value < 100 && speaking) {
    silenceCounter++;
    if (silenceCounter > 10) {
      sendSpeaking(false);
      silenceCounter = 0;
      speaking = false;
      return false;
    }
  }
  if (value > 100 && speaking) {
    silenceCounter = 0;
  }
  if (!speaking) {
    timestamp += kFrameSize;
  }
  return speaking;
}
void DiscordVoiceClient::sendAudioPacket(std::vector<uint8_t>& data,
                                         uint8_t fb) {
  const uint8_t header[12] = {
      0x80,
      fb,
      static_cast<uint8_t>((encode_seq >> (8 * 1)) & 0xff),
      static_cast<uint8_t>((encode_seq >> (8 * 0)) & 0xff),
      static_cast<uint8_t>((timestamp >> (8 * 3)) & 0xff),
      static_cast<uint8_t>((timestamp >> (8 * 2)) & 0xff),
      static_cast<uint8_t>((timestamp >> (8 * 1)) & 0xff),
      static_cast<uint8_t>((timestamp >> (8 * 0)) & 0xff),
      static_cast<uint8_t>((own_ssrc >> (8 * 3)) & 0xff),
      static_cast<uint8_t>((own_ssrc >> (8 * 2)) & 0xff),
      static_cast<uint8_t>((own_ssrc >> (8 * 1)) & 0xff),
      static_cast<uint8_t>((own_ssrc >> (8 * 0)) & 0xff),
  };
  nonce_count++;
  const uint8_t nonce_buff[4] = {
      static_cast<uint8_t>((nonce_count >> (8 * 3)) & 0xff),
      static_cast<uint8_t>((nonce_count >> (8 * 2)) & 0xff),
      static_cast<uint8_t>((nonce_count >> (8 * 1)) & 0xff),
      static_cast<uint8_t>((nonce_count >> (8 * 0)) & 0xff),
  };
  std::memcpy(nonce, nonce_buff, sizeof nonce_buff);
  std::memset(nonce + sizeof header, 0, sizeof nonce - sizeof nonce_buff);
  std::vector<uint8_t> audioDataPacket(sizeof header + data.size() +
                                       crypto_secretbox_MACBYTES);
  std::memcpy(audioDataPacket.data(), header, sizeof header);
  // std::memcpy(nonce, nonce_buff, 4);
  crypto_secretbox_easy(audioDataPacket.data() + sizeof header, &data[0],
                        data.size(), nonce, &secret[0]);
  //  std::cout << audioDataPacket.size() << "\n";
  audioDataPacket.insert(audioDataPacket.end(), nonce_buff, nonce_buff + 4);
  send_remote(audioDataPacket);
  encode_seq++;
  timestamp += kFrameSize;
}
void DiscordVoiceClient::init(std::string guild_id, std::string channel_id) {
  if (state != -1) {
    disconnect();
    return;
  }
  state = 1;
  meta_received = 0;
  this->guild_id = guild_id;
  this->channel_id = channel_id;
}
void DiscordVoiceClient::connect() {
  state = 2;
  std::string target_endpoint = "wss://" + endpoint + "?v=7";
  std::cout << target_endpoint << "\n";
  m_web_socket.setUrl(target_endpoint);
  seq = 0;
  auto* t = this;
  m_web_socket.setOnMessageCallback([t](const ix::WebSocketMessagePtr& msg) {
    std::cout << "new ws msg\n";
    if (msg->type == ix::WebSocketMessageType::Message) {
      t->onWsMessage(msg);
    } else if (msg->type == ix::WebSocketMessageType::Open) {
      t->onWsReady();
    } else if (msg->type == ix::WebSocketMessageType::Error) {
      std::cout << "Connection error: " << msg->errorInfo.reason << std::endl;
    }
  });
  m_web_socket.start();
  std::cout << "starting\n";
}
void DiscordVoiceClient::voiceServerData(DiscordVoiceServerUpdate update) {
  if (state != 1) {
    return;
  }
  endpoint = update.endpoint;
  v_token = update.token;
  meta_received++;
  if (meta_received == 2) {
    connect();
  }
}
bool DiscordVoiceClient::voiceStateData(DiscordVoiceConnectionUpdate update) {
  if (update.user_id == user->id && state == 1) {
    session_id = update.session_id;
    meta_received++;
    if (meta_received == 2) {
      connect();
    }
  } else {
    if (state != -1 && update.user_id != user->id) {
      if (channel_id.length() && update.channel_id == channel_id) {
        connectedIds[update.user_id] = true;
        return true;
      } else if (connectedIds.count(update.user_id) &&
                 !update.channel_id.length()) {
        connectedIds.erase(update.user_id);
        return true;
      }
    }
  }
  return false;
}
void DiscordVoiceClient::onWsReady() {
  if (state != 2)
    return;
  DiscordVoiceAuthPayload auth;
  if (guild_id.length())
    auth.server_id = guild_id;
  else
    auth.server_id = channel_id;
  auth.session_id = session_id;
  auth.token = v_token;
  auth.user_id = user->id;
  state = 3;
  sendMessage(VoiceAuth, auth);
}
void DiscordVoiceClient::setupSocket() {
  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(remote_port);
  servaddr.sin_addr.s_addr = inet_addr(remote_ip.c_str());

  struct timeval tv;
  tv.tv_sec = 10;
  tv.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

  // discovery
  std::stringstream descv_message;
  std::vector<uint8_t> payload;
  payload.push_back(1);
  payload.push_back(0);
  payload.push_back(70);
  payload.push_back(0);
  uint8_t* ssrc_ptr = reinterpret_cast<uint8_t*>(&own_ssrc);
  for (int i = 0; i < 4; ++i)
    payload.push_back(ssrc_ptr[i]);
  for (int i = 0; i < 70 - 8; ++i) {
    payload.push_back(0);
  }
  std::cout << payload.size() << "\n";
  std::vector<uint8_t> received;
  uint8_t tries = 0;
  while (tries < 10) {
    send_remote(payload);

    recv_remote(1024, received);
    if (received.size())
      break;
    else
      std::cout << "len 0?\n";
    tries++;
  }

  std::string recv_ip = std::string((const char*)&received[4]);
  uint16_t port = getShortBigEndian((char*)&received[0], received.size() - 2);
  std::cout << "own_ip: " << recv_ip << ":" << port << "\n";
  own_ip = recv_ip;
  own_port = port;
  DiscordNoOp noop;
  sendMessage(VoiceUnk1, noop);
  DiscordVoiceSelectionMessage selection;
  selection.protocol = "udp";
  selection.address = own_ip;
  selection.port = own_port;
  selection.mode = "xsalsa20_poly1305_lite";
  sendMessage(VoiceHbRequest, selection);
  sendSpeaking(false);
  DiscordVoiceOp12Message op12Message;
  op12Message.audio_ssrc = own_ssrc;
  sendMessage(VoiceUnk2, op12Message);
}
void DiscordVoiceClient::receive_loop() {
  RtAudio dac;
  if (dac.getDeviceCount() < 1) {
    std::cout << "no device\n";
  }
  uint32_t bufferFrames = kFrameSize;
  RtAudio::StreamParameters oParams;
  oParams.deviceId = dac.getDefaultOutputDevice();
  oParams.nChannels = 2;
  oParams.firstChannel = 0;
  int closeCounter = 0;
  bool started = false;
  while (state != 10) {
    std::vector<uint8_t> received;
    recv_remote(1920, received);
    if (received.size() < 12) {
      std::cout << " smaller 12\n";
      continue;
    }

    if (received[1] == 0xc9) {
      continue;
    }

    uint32_t* int_ptr = reinterpret_cast<uint32_t*>(&received[4]);
    uint32_t ssrc = __builtin_bswap32(int_ptr[1]);
    if (ssrc == own_ssrc)
      continue;
    if (decoders_map.count(ssrc)) {
      uint32_t timestamp = __builtin_bswap32(int_ptr[0]);
      std::vector<uint8_t> header =
          std::vector(received.begin(), received.begin() + 12);
      uint16_t* initptr = reinterpret_cast<uint16_t*>(&header[0]);
      uint16_t seq = __builtin_bswap32(initptr[1]);
      std::memcpy(nonce, received.data() + (received.size() - 4), 4);
      std::vector<uint8_t> outBuffer(received.size() - 16);
      int res = crypto_secretbox_open_easy(outBuffer.data(), &received[12],
                                           outBuffer.size(), nonce, &secret[0]);
      if (res != 0) {
        continue;
      }
      outBuffer.erase(
          outBuffer.begin() + (outBuffer.size() - crypto_secretbox_MACBYTES),
          outBuffer.begin() + outBuffer.size());
      uint16_t offset = 0;
      auto* decoder = decoders_map[ssrc];

      if (outBuffer[0] == 0xbe && outBuffer[1] == 0xde &&
          outBuffer.size() > 4) {
        offset = 4;
        initptr = reinterpret_cast<uint16_t*>(&outBuffer[0]);
        uint16_t headerExtensionLength = __builtin_bswap16(initptr[1]);
        for (int i = 0; i < headerExtensionLength; ++i) {
          uint8_t b = outBuffer[offset];
          offset++;
          if (b == 0)
            continue;
          offset += 1 + (b >> 4);
        }
        uint8_t b = outBuffer[offset];
        if (b == 0 || b == 2)
          offset++;
      }

      outBuffer.erase(outBuffer.begin(), outBuffer.begin() + offset);
      auto out = decoder->Decode(outBuffer, kFrameSize * 6, false);

      uint8_t* ptr = reinterpret_cast<uint8_t*>(&out[0]);
      playBackBuffer.insert(playBackBuffer.end(), ptr, ptr + (out.size() * 2));
      if (!started && playBackBuffer.size() > ((kFrameSize * 2) * 2) * 4) {
        started = true;
        if (dac.openStream(&oParams, nullptr, RTAUDIO_SINT16, 48000,
                           &bufferFrames, &audio_callback, (void*)this)) {
          std::cout << "failed to open stream\n";
        }
        if (dac.isStreamOpen() == false) {
        }
        if (dac.startStream()) {
        }
      }
    }
  }
  dac.closeStream();
}
void DiscordVoiceClient::send_loop() {
  sendSpeaking(false);
  opus_int16 dummy[kFrameSize * 2];
  std::memset(dummy, 0, sizeof dummy);
  for (int i = 0; i < 3; ++i) {
    std::vector<opus_int16> data(dummy, dummy + kFrameSize * 2);
    auto enc = encoder.Encode(data, kFrameSize);
    sendAudioPacket(enc[0]);
  }
  RtAudio adc;
  RtAudio::StreamParameters iParams;
  iParams.nChannels = 1;
  iParams.deviceId = adc.getDefaultInputDevice();
  auto info = adc.getDeviceInfo(iParams.deviceId);
  uint32_t bufferFrames = kFrameSize;
  uint32_t fs = 48000;
  if (info.currentSampleRate != 48000) {
    if (info.currentSampleRate != 16000) {
      std::cout << "cant resample that\n";
      return;
    }
    silk_resampler_init(&resampler_state, 16000, 48000, 0);
    bufferFrames = kFrameSize / 3;
    fs = 16000;
    needsResample = true;
  }
  iParams.firstChannel = 0;
  auto* t = this;
  if (adc.openStream(NULL, &iParams, RTAUDIO_SINT16, fs, &bufferFrames,
                     &send_callback, (void*)t)) {
    std::cout << "recv error\n";
    return;
  }

  if (adc.startStream()) {
  }
  while (adc.isStreamRunning() && state != 10) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  adc.closeStream();
}
void DiscordVoiceClient::startAudioLoops() {
  std::memset(nonce, 0, sizeof nonce);
  std::cout << "starting audio loops\n";
  auto* t = this;
  send_thread = new std::thread([t]() { t->send_loop(); });
  recv_thread = new std::thread([t]() { t->receive_loop(); });
}
void DiscordVoiceClient::ensureDecoder(std::string id, uint32_t ssrc) {
  std::cout << "ensuring: " << id << ":" << ssrc << "\n";
  bool has = id_ssrc_map.count(id);
  if (has && id_ssrc_map[id] == ssrc)
    return;
  if (has) {
    auto old_ssrc = id_ssrc_map[id];
    if (decoders_map.count(old_ssrc)) {
      opus::Decoder* dec = decoders_map[old_ssrc];
      delete dec;
      decoders_map.erase(old_ssrc);
    }
  }
  opus::Decoder* dec = new opus::Decoder(48000, 2);
  decoders_map[ssrc] = dec;
  id_ssrc_map[id] = ssrc;
}
void DiscordVoiceClient::send_remote(std::vector<uint8_t>& data) {
#ifdef _WIN32
  sendto(sockfd, (const char*)&data[0], data.size(), 0, (SOCKADDR*)&servaddr,
         sizeof(servaddr));
#else
  auto out = sendto(sockfd, (const char*)&data[0], data.size(), MSG_WAITALL,
                    (struct sockaddr*)&servaddr, sizeof(servaddr));
#endif
}
void DiscordVoiceClient::recv_remote(uint32_t size, std::vector<uint8_t>& out) {
  int n;
  socklen_t len;
  uint8_t* recv_buff = new uint8_t[size];
#ifdef _WIN32
  n = recv(sockfd, (char*)recv_buff, size, 0);
#else
  n = recvfrom(sockfd, (char*)recv_buff, size, MSG_WAITALL,
               (struct sockaddr*)&servaddr, &len);

#endif
  out.insert(out.end(), recv_buff, recv_buff + n);
  delete[] recv_buff;
}
void DiscordVoiceClient::sendSpeaking(bool speaking) {
  DiscordVoiceSpeakingMessage e;
  e.ssrc = own_ssrc;
  e.speaking = speaking;
  sendMessage(VoiceSetSpeaking, e);
}
void DiscordVoiceClient::disconnect() {
  if (state == -1)
    return;

  state = 10;
  if (recv_thread) {
    recv_thread->join();
    delete recv_thread;
    recv_thread = nullptr;
  }
  if (hb_thread) {
    hb_thread->join();
    delete hb_thread;
    hb_thread = nullptr;
  }
  if (native_hb_thread) {
    native_hb_thread->join();
    delete native_hb_thread;
    native_hb_thread = nullptr;
  }
  if (send_thread) {
    send_thread->join();

    delete send_thread;
    send_thread = nullptr;
  }

  for (std::map<uint32_t, opus::Decoder*>::iterator it = decoders_map.begin();
       it != decoders_map.end(); ++it) {
    delete it->second;
  }
  decoders_map.clear();
  id_ssrc_map.clear();

  std::memset(nonce, 0, 24);
  nonce_count = 0;
  playBackBuffer.clear();
  secret.clear();
  meta_received = 0;
  state = -1;
  m_web_socket.stop();
  close(sockfd);
  encode_seq = 0;
  timestamp = 0;
  remote_ip = "";
  own_ssrc = 0;
  own_ip = "";
  own_port = 0;
  remote_port = 0;
  sockfd = 0;
  needsResample = false;
  silenceCounter = 0;
  std::cout << "disconnected\n";
}
void DiscordVoiceClient::onWsMessage(const ix::WebSocketMessagePtr& msg) {
  json j = json::parse(msg->str);
  std::cout << "vc ws recv: " << j << "\n";
  auto p = j.get<DiscordVoiceBaseMessage>();
  if (p.opCode == VoiceHbRequest) {
    DiscordVoiceHeartbeatMessage hbMsg;
    sendMessage(VoiceHeartbeat, hbMsg);
    return;
  }
  if (p.opCode == VoiceHeartBeatInit) {
    if (hb_thread != nullptr) {
      return;
    }
    hb_interval = p.data["heartbeat_interval"];
    std::cout << "hb interval: " << hb_interval << "\n";
    {
      DiscordVoiceHeartbeatMessage hbMsg;
      sendMessage(VoiceHeartbeat, hbMsg);
    }
    auto* t = this;
    hb_thread = new std::thread([t]() {
      uint32_t passed = 0;
      while (t->state != 10) {
        ///  std::cout << "inter: " << passed << ":" << t->hb_interval << "\n";
        if (passed >= 15000) {
          DiscordVoiceHeartbeatMessage hbMsg;
          t->sendMessage(VoiceHeartbeat, hbMsg);
          passed = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        passed++;
      }
    });
  } else if (p.opCode == VoiceInitPayload) {
    std::cout << "state: " << state << "\n";
    if (state == 3) {
      DiscordVoiceInitPayload pp;
      pp.fromJson(p.data);
      remote_ip = pp.ip;
      remote_port = pp.port;
      own_ssrc = pp.ssrc;
      native_hb = pp.hb_interval;
      setupSocket();
      state = 4;
    }
  } else if (p.opCode == VoiceAuthResponse) {
    if (state == 4) {
      DiscordVoiceSelectionResponsePayload resp;
      resp.fromJson(p.data);
      state = 5;
      secret = resp.secret;

      auto* t = this;
      std::vector<uint8_t> lol = {0, 0, 0, 0, 0, 0, 0};
      this->sendAudioPacket(lol, 0xf8);
      native_hb_thread = new std::thread([t]() {
        uint32_t passed = 0;
        while (t->state != 10) {
          if (passed >= t->native_hb) {
            std::vector<uint8_t> bytes = {0xC9, 0, 0, 0, 0, 0, 0, 0, 0};
            t->send_remote(bytes);
            passed = 0;
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      });
      startAudioLoops();
    }
  } else if (p.opCode == VoiceSetSpeaking) {
    DiscordVoiceSpeakingMessage pp;
    pp.fromJson(p.data);
    if (pp.ssrc && pp.user_id.length())
      ensureDecoder(pp.user_id, pp.ssrc);
  }
}
void DiscordVoiceClient::sendMessage(DiscordVoiceOpCode op,
                                     DiscordMessage& msg) {
  json d = msg.getJson();
  DiscordVoiceBaseMessage m = {op, d};
  json toSend = m;
  std::cout << toSend << "\n";
  m_web_socket.send(toSend.dump());
};

uint16_t DiscordVoiceClient::getShortBigEndian(char* arr, int offset) {
  return (short)((arr[offset] & 0xff << 8) | (arr[offset + 1] & 0xff));
};