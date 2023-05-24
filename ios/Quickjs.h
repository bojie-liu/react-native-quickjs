#ifdef __cplusplus
#import "react-native-quickjs.h"
#endif

#ifdef RCT_NEW_ARCH_ENABLED
#import "RNQuickjsSpec.h"

@interface Quickjs : NSObject <NativeQuickjsSpec>
#else
#import <React/RCTBridgeModule.h>

@interface Quickjs : NSObject <RCTBridgeModule>
#endif

@end
