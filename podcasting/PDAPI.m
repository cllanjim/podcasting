//
//  PDAPI.m
//  podcasting
//
//  Created by houxh on 15/6/24.
//  Copyright (c) 2015年 beetle. All rights reserved.
//

#import "PDAPI.h"
#import "TAHttpOperation.h"

static NSString *apiURL = @"http://192.168.1.101:8081";

@implementation PDAPI

+(NSOperation*)startChan:(int64_t)chan_id sdp:(NSString*)sdp success:(void (^)())success fail:(void (^)())fail {
    IMHttpOperation *request = [IMHttpOperation httpOperationWithTimeoutInterval:60];
    request.targetURL = [apiURL stringByAppendingString:@"/chans/start"];
    request.method = @"POST";
    
    NSDictionary *dict = @{@"sdp":sdp, @"chan_id":[NSNumber numberWithLongLong:chan_id]};
    
    NSData *data = [NSJSONSerialization dataWithJSONObject:dict options:0 error:nil];
    request.postBody = data;
    
    NSMutableDictionary *headers = [NSMutableDictionary dictionaryWithObject:@"application/json" forKey:@"Content-Type"];
    request.headers = headers;
    
    request.successCB = ^(IMHttpOperation*commObj, NSURLResponse *response, NSData *data) {
        NSInteger statusCode = [(NSHTTPURLResponse*)response statusCode];
        if (statusCode != 200) {
            fail();
        } else {
            success();
        }
    };
    request.failCB = ^(IMHttpOperation*commObj, IMHttpOperationError error) {
        fail();
    };
    [[NSOperationQueue mainQueue] addOperation:request];
    return request;
}


+(NSOperation*)stopChan:(int64_t)chan_id success:(void (^)())success fail:(void (^)())fail {
    IMHttpOperation *request = [IMHttpOperation httpOperationWithTimeoutInterval:60];
    request.targetURL = [NSString stringWithFormat:@"%@/chans/stop?id=%lld", apiURL, chan_id];
    request.method = @"POST";

    
    request.successCB = ^(IMHttpOperation*commObj, NSURLResponse *response, NSData *data) {
        NSInteger statusCode = [(NSHTTPURLResponse*)response statusCode];
        if (statusCode != 200) {
            fail();
        } else {
            success();
        }
    };
    request.failCB = ^(IMHttpOperation*commObj, IMHttpOperationError error) {
        fail();
    };
    [[NSOperationQueue mainQueue] addOperation:request];
    return request;
}

@end
