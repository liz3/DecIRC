#import "../global-protocol-handler.h"
#import <Foundation/Foundation.h>

@interface DecMacProtocolHandler : NSObject
+ (void)handleGetURLEvent:(NSAppleEventDescriptor *)event;
@end