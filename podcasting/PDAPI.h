//
//  PDAPI.h
//  podcasting
//
//  Created by houxh on 15/6/24.
//  Copyright (c) 2015年 beetle. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface PDAPI : NSObject
+(NSOperation*)startChan:(int64_t)chan_id sdp:(NSString*)sdp success:(void (^)())success fail:(void (^)())fail;
+(NSOperation*)stopChan:(int64_t)chan_id success:(void (^)())success fail:(void (^)())fail;
@end
