#import "Notifications.h"

@implementation MacNotifs

int sendNotifToObjc(const char* title, const char* body) {
  MacNotifs* instance = [[MacNotifs alloc] init];
  int result = [instance sendNotif:title body:body];
  [instance release];
  return result;
}

- (int)sendNotif:(const char*)title body:(const char*)body {
  NSString* nsTitle = [NSString stringWithUTF8String:title];
  NSString* nsBody = [NSString stringWithUTF8String:body];

  NSUserNotification* notification = [[NSUserNotification alloc] init];
  [notification setTitle:nsTitle];
  [notification setInformativeText:nsBody];
    [notification setSoundName:NSUserNotificationDefaultSoundName];
  NSUserNotificationCenter* center =
      [NSUserNotificationCenter defaultUserNotificationCenter];
  [center deliverNotification:notification];
  return 0; 
}
- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center
     shouldPresentNotification:(NSUserNotification*)notification {
  return YES;
}

@end