//
//  FFPlayer.m
//  podcasting
//
//  Created by houxh on 15/10/9.
//  Copyright (c) 2015年 beetle. All rights reserved.
//

#import "FFPlayer.h"
#import "video_render_ios_view.h"
#import "ff_player.h"
#import "libavformat/avformat.h"


static void render_func(void *data, struct AVFrame *frame) {
    NSLog(@"w:%d h:%d", frame->width, frame->height);
    VideoRenderIosView *render = (__bridge VideoRenderIosView*)data;
    [render renderFrame:frame];
    [render presentFramebuffer];
}


@interface FFPlayer()

@property (weak, nonatomic) VideoRenderIosView *renderView;
@property(nonatomic)struct FFPlayer *player;
@property(nonatomic)NSTimer *timer;


@end


@implementation FFPlayer

-(id)initWithRenderView:(id)render {
    self = [super init];
    if (self) {
        self.renderView = render;
        struct FFPlayer *player = (struct FFPlayer*)malloc(sizeof(struct FFPlayer));
        ff_player_init(player);
        
        player->render = render_func;
        player->render_data = (__bridge void*)self.renderView;
        
        self.player = player;
    }
    return  self;
}

-(void)dealloc {
    free(self.player);
}

-(void)play:(NSString*)url {
    self.timer = [NSTimer scheduledTimerWithTimeInterval:REFRESH_RATE target:self selector:@selector(onRefresh) userInfo:nil repeats:NO];
    
    self.player->input_filename = strdup([url UTF8String]);
    ff_player_play(self.player);
}

-(void)stop {
    ff_player_stop(self.player);
    free((void*)(self.player->input_filename));
    self.player->input_filename = NULL;
    
    [self.timer invalidate];
    self.timer = NULL;
}

-(void)onRefresh {
    //    NSLog(@"on refresh");
    double remaining_time = REFRESH_RATE;
    ff_player_refresh_video(self.player, &remaining_time);
    
    self.timer = [NSTimer scheduledTimerWithTimeInterval:remaining_time target:self selector:@selector(onRefresh) userInfo:nil repeats:NO];
    
}

@end
