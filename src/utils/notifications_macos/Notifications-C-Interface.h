#ifndef NOTIFICATION_C_INTERFACE_H__
#define NOTIFICATION_C_INTERFACE_H__
extern "C" {
int sendNotifToObjc(const char* title, const char* body);
}
#endif
