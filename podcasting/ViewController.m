//
//  ViewController.m
//  podcasting
//
//  Created by houxh on 15/5/17.
//  Copyright (c) 2015年 beetle. All rights reserved.
//

#import "ViewController.h"
#import "video_capture_ios_objc.h"
#import "video_render_ios_view.h"
#import "Broadcaster.h"
#import "PDAPI.h"

@interface ViewController ()
@property(nonatomic) Broadcaster *broadcaster;
@property(nonatomic) VideoRenderIosView *render;

@property(nonatomic) BOOL started;

@end

@implementation ViewController

- (IBAction)startCapture:(id)sender {
    
    NSLog(@"start capture");
    
    if (self.started) {
        return;
    }
    
    self.started = YES;
    
    if (self.broadcaster == nil) {
        NSString *token = @"000000000000000000000000000000";
        NSString *deviceID = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
        
        self.broadcaster = [[Broadcaster alloc] initWithChanID:0 deviceID:deviceID token:token useUDP:NO];
        self.broadcaster.render = self.render;
    }
    
    
    [self.broadcaster start];
    
//    NSString *sdp = @"";
//    NSLog(@"sdp:%@", sdp);
//    [PDAPI startChan:0
//                 sdp:sdp
//             success:^{
//                 NSLog(@"start chan success");
//                 if (!self.broadcaster.isRunning) {
//                     [self.broadcaster start];
//                 }
//             }
//                fail:^{
//                 NSLog(@"start chan error");
//                    
//                    self.broadcaster = nil;
//                    self.started = NO;
//                }];
}

- (IBAction)stopCapture:(id)sender {
    if (!self.started) {
        return;
    }
    self.started = NO;
    
    
    if (self.broadcaster.isRunning) {
        [self.broadcaster stop];
    }
    
    self.broadcaster = nil;
    
    [PDAPI stopChan:0
            success:^{
                NSLog(@"stop chan success");
            }
               fail:^{
                   NSLog(@"stop chan error");
               }];
    return;
}


- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    
    CGRect r = CGRectMake(0, 30, 352, 288);
    VideoRenderIosView *render = [[VideoRenderIosView alloc] initWithFrame:r];
    [self.view addSubview:render];
    
    
    if (![render createContext]) {
        exit(1);
    }
    
    self.render = render;
    

    
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    if (toInterfaceOrientation == UIInterfaceOrientationLandscapeLeft || toInterfaceOrientation == UIInterfaceOrientationLandscapeRight) {
        self.render.frame = self.view.bounds;
    } else {
        CGRect r = CGRectMake(0, 30, 352, 288);
        self.render.frame = r;
    }
}





@end
