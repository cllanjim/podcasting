//
//  main.m
//  podcasting
//
//  Created by houxh on 15/5/17.
//  Copyright (c) 2015年 beetle. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#include "libavformat/avformat.h"
int main(int argc, char * argv[]) {
    av_register_all();
    avformat_network_init();
    
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
