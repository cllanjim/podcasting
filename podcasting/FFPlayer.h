//
//  FFPlayer.h
//  podcasting
//
//  Created by houxh on 15/10/9.
//  Copyright (c) 2015年 beetle. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface FFPlayer : NSObject
-(id)initWithRenderView:(id)render;
-(void)play:(NSString*)url;
-(void)stop;
@end
