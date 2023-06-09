//
//  TTIModule.m
//  QuickjsExample
//
//  Created by liubojie on 2023/6/9.
//

#import "TTIModule.h"

#import <React/RCTRootView.h>

NSTimeInterval tti;

NSTimeInterval onCreateTimestamp;

NSTimeInterval componentDidMount;

@implementation TTIModule
RCT_EXPORT_MODULE()

// Example method
// See // https://reactnative.dev/docs/native-modules-ios
RCT_REMAP_METHOD(componentDidMount,
                 componentDidMount:(double)timestamp)
{
  if (componentDidMount == 0) {
    componentDidMount = [[NSDate date] timeIntervalSince1970] - onCreateTimestamp;
    [self printTTI];
  }
}

- (instancetype)init {
    if (self = [super init]) {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(contentDidAppear:) name:RCTContentDidAppearNotification object:nil];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:RCTContentDidAppearNotification
                                                  object:nil];
}

- (void)printTTI {
  if (componentDidMount != 0 && tti != 0) {
    NSLog(@"TTI: %f ComponentDidMount: %f", tti * 1000, componentDidMount * 1000);
  }
}

#pragma mark - Notification

- (void)contentDidAppear:(NSNotification *)notification {
    if (!notification) {
        return;
    }
    tti = [[NSDate date] timeIntervalSince1970] - onCreateTimestamp;
    [self printTTI];
}

@end

