#include "notifications.h"
#ifdef _WIN32
#include <locale>
#include <codecvt>
#endif

void Notifications::sendNotification(const std::string& title, const std::string& body) {
#ifdef _WIN32
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  std::wstring wideTitle = converter.from_bytes(title);
  std::wstring wideBody = converter.from_bytes(body);
  WinToastTemplate templ(WinToastTemplate::Text02);
  templ.setTextField(wideTitle, WinToastTemplate::FirstLine);
  templ.setTextField(wideBody, WinToastTemplate::SecondLine);
  WinToast::instance()->showToast(templ, new WinToastHandlerExample());
#endif
#ifdef __APPLE__
  sendNotifToObjc((const char*)title.c_str(), (const char*)body.c_str());
#endif
#ifdef __linux__
  NotifyNotification* n =
      notify_notification_new(title.c_str(), body.c_str(), 0);
  notify_notification_show(n, 0);
#endif
}
void Notifications::init() {
#ifdef _WIN32
  if (!WinToast::isCompatible()) {
    std::wcerr << L"Error, your system in not supported!" << std::endl;
  }
  WinToast::instance()->setAppName(L"Dec");
  const auto aumi =
      WinToast::configureAUMI(L"Liz3", L"dec", L"dec", L"20161006");
  WinToast::instance()->setAppUserModelId(aumi);
  WinToast::WinToastError error;
  const bool succedded = WinToast::instance()->initialize(&error);
  if (!succedded) {
    std::wcout << L"Error, could not initialize the lib. Error number: "
               << error << std::endl;
  }

#endif
#ifdef __linux__
  notify_init("DecIrc");
#endif
}
#ifdef _WIN32
void WinToastHandlerExample::toastActivated() const {}
void WinToastHandlerExample::toastActivated(int x) const {}
void WinToastHandlerExample::toastDismissed(
    WinToastDismissalReason state) const {}
void WinToastHandlerExample::toastFailed() const {}
#endif