#ifndef DEC_NOTIFICATIONS_H
#define DEC_NOTIFICATIONS_H
#include <string>
#ifdef _WIN32
#include "../../third-party/wintoast/wintoastlib.h"
using namespace WinToastLib;

class WinToastHandlerExample : public IWinToastHandler {
 public:
    WinToastHandlerExample() {

    };
    // Public interfaces
    void toastActivated() const override;
    void toastActivated(int) const override;
    void toastDismissed(WinToastDismissalReason state) const override;
    void toastFailed() const override;
 };
#endif
#ifdef __APPLE__
#include "notifications_macos/Notifications-C-Interface.h"
 #endif

class Notifications {
public:
    static void sendNotification(std::string& title, std::string body);
    static void init();
};
#endif