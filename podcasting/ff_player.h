//
//  ff_player.h
//  ffplay
//
//  Created by houxh on 15/6/9.
//  Copyright (c) 2015年 beetle. All rights reserved.
//

#ifndef __ffplay__ff_player__
#define __ffplay__ff_player__

#include <stdio.h>

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

typedef struct VideoState VideoState;
typedef struct AVInputFormat AVInputFormat;
enum ShowMode {
    SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
};

struct AVFrame;

typedef void (*RenderFrame)(void *user_data, struct AVFrame *frame);
typedef struct FFPlayer {
    VideoState *is;
    
    RenderFrame render;
    void *render_data;
    
    ///* options specified by the user */
    AVInputFormat *file_iformat;
    const char *input_filename;

    int default_width;
    int default_height;
    int audio_disable;
    int video_disable;
    int subtitle_disable;
    int seek_by_bytes;
    int display_disable;
    int show_status;
    int av_sync_type;
    int64_t start_time;
    int64_t duration;
    int fast;
    int genpts;
    int lowres;
    int decoder_reorder_pts;
    int autoexit;
    int loop;
    int framedrop;
    int infinite_buffer;
    enum ShowMode show_mode;
    const char *audio_codec_name;
    const char *subtitle_codec_name;
    const char *video_codec_name;
    double rdftspeed;
};


void ff_init();
void ff_player_init(struct FFPlayer *player);
int ff_player_play(struct FFPlayer *player);
int ff_player_pause(struct FFPlayer *player);
int ff_player_stop(struct FFPlayer *player);
void ff_player_refresh_video(struct FFPlayer *player, double *remaining_time);
#endif /* defined(__ffplay__ff_player__) */
