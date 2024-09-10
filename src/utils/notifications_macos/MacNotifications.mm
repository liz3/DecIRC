#import "Notifications.h"

@implementation MacNotifs

int sendNotifToObjc(const char* title, const char* body, int sound) {
  MacNotifs* instance = [[MacNotifs alloc] init];
  int result = [instance sendNotif:title body:body sound: sound];
  [instance release];
  return result;
}

- (int)sendNotif:(const char*)title body:(const char*)body sound:(int)sound {
  NSString* nsTitle = [NSString stringWithUTF8String:title];
  NSString* nsBody = [NSString stringWithUTF8String:body];

  NSUserNotification* notification = [[NSUserNotification alloc] init];
  [notification setTitle:nsTitle];
  [notification setInformativeText:nsBody];
  if(sound == 1){
    [notification setSoundName:NSUserNotificationDefaultSoundName];
  }
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