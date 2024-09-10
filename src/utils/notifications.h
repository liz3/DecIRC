#ifndef DEC_NOTIFICATIONS_H
#define DEC_NOTIFICATIONS_H
#include <string>
#ifdef _WIN32
#include "../../third-party/wintoast/wintoastlib.h"
using namespace WinToastLib;
class WinToastHandlerExample : public IWinToastHandler {
 public:
  WinToastHandlerExample(){};
  void toastActivated() const override;
  void toastActivated(int) const override;
  void toastDismissed(WinToastDismissalReason state) const override;
  void toastFailed() const override;

};

#endif
#ifdef __APPLE__
#include "notifications_macos/Notifications-C-Interface.h"
#endif
#ifdef __linux__
#include <libnotify/notify.h>
#endif

class Notifications {
 public:
  static void sendNotification(const std::string& title, const std::string& body, bool sound = false);
  static void init();
};
#endif