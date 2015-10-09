//
//  Broadcaster.h
//  podcasting
//
//  Created by houxh on 15/5/23.
//  Copyright (c) 2015年 beetle. All rights reserved.
//

#import <Foundation/Foundation.h>

@class VideoRenderIosView;
@interface Broadcaster : NSObject
@property(atomic, getter=isRunning) BOOL running;
@property(nonatomic) VideoRenderIosView *render;

-(id)initWithChanID:(int64_t)chanID deviceID:(NSString*)deviceID token:(NSString*)token useUDP:(BOOL)useUDP;

-(void)start;
-(void)stop;
@end
