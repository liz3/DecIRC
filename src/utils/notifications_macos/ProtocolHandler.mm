#import "ProtocolHandler.h"
#import <Carbon/Carbon.h>

@implementation DecMacProtocolHandler
+ (void)handleGetURLEvent:(NSAppleEventDescriptor *)event
{
  global_handle_protocol([[[event paramDescriptorForKeyword:keyDirectObject] stringValue] UTF8String]);
}
@end

void global_start_protocol_handler(void) {
  NSLog(@"we are in objective c starting the url handler\n");
  NSAppleEventManager *appleEventManager = [NSAppleEventManager sharedAppleEventManager];
    [appleEventManager setEventHandler:[DecMacProtocolHandler class]
                           andSelector:@selector(handleGetURLEvent:)
                         forEventClass:kInternetEventClass andEventID:kAEGetURL];
}