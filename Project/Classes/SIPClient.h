//
//  SIPClient.h
//  ShahbazTestSIP
//
//  Created by shahbaz tariq on 12/7/18.
//  Copyright © 2018 shahbaz tariq. All rights reserved.
//

#import <Foundation/Foundation.h>

@class SIPCall;
@protocol SIPClientDelegate;

NS_ASSUME_NONNULL_BEGIN

@interface SIPClient : NSObject

+ (instancetype)sharedInstance;

@property (nonatomic, weak) id<SIPClientDelegate> delegate;
@property (nonatomic, copy, readonly) NSString *username;

- (void)configureWithUsername:(NSString *)username password:(NSString *)password domain:(NSString *)domain;

- (void)prepareToReceiveCall:(void (^)(BOOL success))completionHandler;

- (void)answerCall:(SIPCall *)call;
- (void)ringingCall:(SIPCall *)call;

- (BOOL)placeCall:(SIPCall *)call;
- (void)endCall:(SIPCall *)call;

@end

NS_ASSUME_NONNULL_END

@protocol SIPClientDelegate <NSObject>

- (void)sipClient:(SIPClient *)sipClient didReceiveIncomingCall:(SIPCall *)call;

@end
