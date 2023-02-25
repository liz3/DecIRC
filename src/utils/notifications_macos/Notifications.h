#import "Notifications-C-Interface.h"
#import <Foundation/NSObject.h>
#import <Foundation/NSString.h>
#import <Foundation/NSDate.h>

#import <Foundation/NSUserNotification.h>
#import <stdio.h>

@interface MacNotifs : NSObject <NSUserNotificationCenterDelegate> {
}

- (int)sendNotif:(const char*)title body:(const char*)body;
- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center
     shouldPresentNotification:(NSUserNotification*)notification;

@end
