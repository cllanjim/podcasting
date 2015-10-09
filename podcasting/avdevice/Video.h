//
//  Video.h
//  podcasting
//
//  Created by houxh on 15/5/18.
//  Copyright (c) 2015年 beetle. All rights reserved.
//

#ifndef podcasting_Video_h
#define podcasting_Video_h


// Raw video types
enum RawVideoType
{
    kVideoI420     = 0,
    kVideoYV12     = 1,
    kVideoYUY2     = 2,
    kVideoUYVY     = 3,
    kVideoIYUV     = 4,
    kVideoARGB     = 5,
    kVideoRGB24    = 6,
    kVideoRGB565   = 7,
    kVideoARGB4444 = 8,
    kVideoARGB1555 = 9,
    kVideoMJPEG    = 10,
    kVideoNV12     = 11,
    kVideoNV21     = 12,
    kVideoBGRA     = 13,
    kVideoUnknown  = 99
};

// Enums
enum VideoCaptureRotation
{
    kCameraRotate0 = 0,
    kCameraRotate90 = 5,
    kCameraRotate180 = 10,
    kCameraRotate270 = 15
};


struct VideoCaptureCapability
{
    int32_t width;
    int32_t height;
    int32_t maxFPS;
    //    int32_t expectedCaptureDelay;
    enum RawVideoType rawType;
    //    bool interlaced;
};

typedef struct AVFrame AVFrame;
typedef struct AVPacket AVPacket;



@protocol ICaptureDelegate <NSObject>

-(int32_t)IncomingFrame:(AVFrame*)frame;

@end


@protocol ICapture <NSObject>
@property(nonatomic, weak) id<ICaptureDelegate> delegate;

- (BOOL)startCaptureWithCapability:
(const struct VideoCaptureCapability)capability;
- (BOOL)stopCapture;
@end

@protocol IVideoEncoder <NSObject>
-(BOOL)encoder:(AVFrame*)frame packet:(AVPacket*)packet;

@end

@protocol IVideoDecoder <NSObject>

-(BOOL)decoder:(AVPacket*)packet frame:(AVFrame*)frame;

@end

@protocol IVideoRender <NSObject>

-(void)render:(AVFrame*)frame;

@end


#endif
