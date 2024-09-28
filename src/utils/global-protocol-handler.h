#ifndef DEC_GLOBAL_PROTOCOL_HANDLER_H
#define DEC_GLOBAL_PROTOCOL_HANDLER_H

extern "C" {
  void global_handle_protocol(const char* url);
#ifdef __APPLE__
  void global_start_protocol_handler();
#endif
}


#endif