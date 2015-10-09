//
//  TVViewController.m
//  podcasting
//
//  Created by houxh on 15/10/9.
//  Copyright (c) 2015年 beetle. All rights reserved.
//

#import "TVViewController.h"
#import "video_render_ios_view.h"

#include "ff_player.h"
#include "libavformat/avformat.h"
#import "FFPlayer.h"


//#define HOST "192.168.33.10"
#define HOST "121.42.143.50"


@interface TVViewController ()
@property (weak, nonatomic) IBOutlet VideoRenderIosView *renderView;

@property(nonatomic, getter=isListening) BOOL listening;

@property(nonatomic) FFPlayer *player;

@end

@implementation TVViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    if (![self.renderView createContext]) {
        exit(1);
    }
    
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)startPlay:(id)sender {
    if (self.isListening) {
        return;
    }
    self.player = [[FFPlayer alloc] initWithRenderView:self.renderView];
    NSString *path = [NSString stringWithFormat:@"rtmp://%s/mytv/%lld.flv", HOST, 0ll];
    
    [self.player play:path];
    
    self.listening = YES;
}

- (IBAction)stopPlay:(id)sender {
    if (!self.isListening) {
        return;
    }
    
    [self.player stop];
    self.player = nil;
    
    self.listening = NO;
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
