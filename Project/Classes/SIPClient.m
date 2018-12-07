//
//  SIPClient.m
//  ShahbazTestSIP
//
//  Created by shahbaz tariq on 12/7/18.
//  Copyright © 2018 shahbaz tariq. All rights reserved.
//

#import "SIPClient.h"
#import "SIPCall.h"
#import <AVFoundation/AVFoundation.h>
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>

#import <arpa/inet.h>
#import <mach/mach_init.h>
#import <netinet/in.h>
#import <objc/runtime.h>
#import <pj_extras.h>
#import <pjsip_talkhome.h>
#import <pjsua-lib/pjsua.h>
#import <pthread.h>
#import <sys/socket.h>

#ifndef PJ_HAS_IPV6
#error PJ_HAS_IPV6 not defined, please re-build pjsip
#endif

@implementation SIPClient

@end
