//
//  Broadcaster.m
//  podcasting
//
//  Created by houxh on 15/5/23.
//  Copyright (c) 2015年 beetle. All rights reserved.
//

#import "Broadcaster.h"
#import "video_capture_ios_objc.h"
#import "device_info_ios_objc.h"
#import "video_render_ios_view.h"
#include "audio_device_ios.h"

#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>


extern "C" {
#include "util.h"
    
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavformat/avio_internal.h"
#include "libavformat/rtpdec.h"
    
    
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"

    
}

//static const char *filename = "rtmp://192.168.1.103/mytv/0.flv";
static const char *filename = "rtmp://121.42.143.50/mytv/0.flv";



#define CAPTURE_WIDTH 640
#define CAPTURE_HEIGHT 480

#define WIDTH 640
#define HEIGHT 480

//#define WIDTH 352
//#define HEIGHT 288

//#define WIDTH 1280
//#define HEIGHT 720

#define FPS 24

class AudioDataTransport;

@interface Packet : NSObject
@property(nonatomic) AVPacket *packet;
@property(nonatomic, getter=isAudio) BOOL audio;
@end

@implementation Packet

-(id)init {
    self = [super init];
    if (self) {
        AVPacket *pkt = (AVPacket*)malloc(sizeof(AVPacket));
        av_init_packet(pkt);
        self.packet = pkt;
    }
    return self;
}

-(void)dealloc {
    av_free_packet(self.packet);
    free(self.packet);
}

@end

static AVFormatContext* openRTMP(char *filename, int video_id, int audio_id, int width, int height, int sample_rate, int channels) {
    AVFormatContext *ctx;
    AVStream *st;
    AVCodec *codec;

    
    ctx = avformat_alloc_context();
    if (!ctx) {
        exit(1);
    }
    printf("file name:%s\n", filename);
    ctx->oformat = av_guess_format("flv", NULL, NULL);
    
    snprintf(ctx->filename, 1024, "%s", filename);

    codec = avcodec_find_encoder(AVCodecID(video_id));
    st = avformat_new_stream(ctx, codec);
    st->id = ctx->nb_streams-1;
    st->codec->pix_fmt = AV_PIX_FMT_YUV420P;
    st->codec->width = width;
    st->codec->height = height;
    st->codec->time_base = (AVRational){1,1000};
    
    AVDictionary *opts = NULL;
    if (video_id == AV_CODEC_ID_H264) {
        av_dict_set(&opts, "profile", "baseline", 0);
    }
    
    if (video_id == AV_CODEC_ID_VP8) {
        av_dict_set_int(&opts, "crf", 20, 0);
        av_dict_set_int(&opts, "cpu-used", 16, 0);
    }

    
    av_dict_set(&opts, "flags", "+global_header", 0);
    /* open it */
    if (avcodec_open2(st->codec, codec, &opts) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
    av_dict_free(&opts);
    opts = NULL;
    
    codec = avcodec_find_encoder(AVCodecID(audio_id));
    st = avformat_new_stream(ctx, codec);
    st->id = ctx->nb_streams-1;
    st->codec->sample_fmt = AV_SAMPLE_FMT_FLTP;
    st->codec->sample_rate = sample_rate;
    st->codec->channels = channels;
    st->codec->channel_layout = av_get_default_channel_layout(channels);
    st->codec->time_base = (AVRational){1,1000};
    
    
    av_dict_set(&opts, "flags", "+global_header", 0);
    av_dict_set(&opts, "strict", "experimental", 0);

    if (avcodec_open2(st->codec, codec, &opts) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
    av_dict_free(&opts);
    
    return ctx;
}

static void closeRTMP(AVFormatContext *ctx) {
    for (int i = ctx->nb_streams - 1; i >= 0; i--) {
        avcodec_close(ctx->streams[i]->codec);
    }
    
    avformat_free_context(ctx);
}



@interface Broadcaster()<VideoCaptureDelegate> {
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    AVFrame *_captureFrame;


    pthread_mutex_t _queueMutex;
    pthread_cond_t _queueCond;
    
    pthread_t _sendTid;
    
    SwsContext *_sws;
    pthread_t _tid;
    
    AVIOInterruptCB _icb;
}

@property(atomic) BOOL encoding;
@property(atomic) BOOL sending;

@property(nonatomic)AudioDataTransport *audioTransport;
@property(nonatomic)webrtc::AudioDeviceIPhone *audioDevice;
@property(nonatomic)VideoCaptureIosObjC *videoDevice;

@property(nonatomic) double beginTS;

@property(nonatomic) AVFormatContext *ctx;
@property(nonatomic) AVCodecContext *videoCodecContext;
@property(nonatomic) AVCodecContext *audioCodecContext;

@property(nonatomic) NSMutableArray *packetQueue;

-(void)onRecordData:(const void*)samples count:(int)count perSample:(int)bytePerSample
           channels:(int)channels samplesPerSecond:(int)samplesPerSecond;


-(void)runEncoding;
-(void)runSendLoop;

@end

class AudioDataTransport:public webrtc::AudioTransport {
public:
    
    AudioDataTransport(Broadcaster *broadcaster):_broadcaster(broadcaster) {

        
    }
    virtual int32_t RecordedDataIsAvailable(const void* audioSamples,
                                            const uint32_t nSamples,
                                            const uint8_t nBytesPerSample,
                                            const uint8_t nChannels,
                                            const uint32_t samplesPerSec,
                                            const uint32_t totalDelayMS,
                                            const int32_t clockDrift,
                                            const uint32_t currentMicLevel,
                                            const bool keyPressed,
                                            uint32_t& newMicLevel) {
        
        [_broadcaster onRecordData:audioSamples count:nSamples  perSample:nBytesPerSample channels:nChannels samplesPerSecond:samplesPerSec];
        return 0;
    }
    
    virtual int32_t NeedMorePlayData(const uint32_t nSamples,
                                     const uint8_t nBytesPerSample,
                                     const uint8_t nChannels,
                                     const uint32_t samplesPerSec,
                                     void* audioSamples,
                                     uint32_t& nSamplesOut) {
        

        return 0;
    }
    
private:
    __weak Broadcaster *_broadcaster;

};


static void* encode_thread(void* arg) {
    
    Broadcaster *b = (__bridge Broadcaster*)arg;
    [b runEncoding];
    return 0;
}

static void* send_thread(void *arg) {
    Broadcaster *b = (__bridge Broadcaster*)arg;
    [b runSendLoop];
    return 0;
}

static int interupt_callback(void *opaque) {
    Broadcaster *b = (__bridge Broadcaster*)opaque;
    return b.sending ? 0 : 1;
}

@implementation Broadcaster

-(id)initWithChanID:(int64_t)chanID deviceID:(NSString*)deviceID token:(NSString*)token useUDP:(BOOL)useUDP {
    self = [super init];
    
    int res;
    if (self) {
        self.packetQueue = [NSMutableArray array];
        
        webrtc::AudioDeviceIPhone *audioDevice = new  webrtc::AudioDeviceIPhone(0);

        res = audioDevice->Init();
        if (res != 0) {
            return nil;
        }
        
        self.audioDevice = audioDevice;
        self.audioTransport = new AudioDataTransport(self);
        self.audioDevice->RegisterAudioCallback(self.audioTransport);

        self.audioDevice->SetRecordingDevice(0);
        res = self.audioDevice->InitRecording();

        if (res != 0) {
            return nil;
        }
        
        self.videoDevice = [[VideoCaptureIosObjC alloc] initWithCaptureID:0];
        self.videoDevice.delegate = self;
        
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        
        int result = pthread_mutex_init(&_mutex, &attr);
        if (result != 0) {
            exit(1);
        }


        result = pthread_cond_init(&_cond, NULL);
        if (result != 0) {
            exit(1);
        }
        
        result = pthread_mutex_init(&_queueMutex, &attr);
        if (result != 0) {
            exit(1);
        }
        
        
        result = pthread_cond_init(&_queueCond, NULL);
        if (result != 0) {
            exit(1);
        }
        
        pthread_mutexattr_destroy(&attr);
        
        _icb.callback = interupt_callback;
        _icb.opaque = (__bridge void*)self;
 
        AVFormatContext *ctx = openRTMP((char*)filename, AV_CODEC_ID_H264, AV_CODEC_ID_AAC, WIDTH, HEIGHT, 44100, 1);
        self.videoCodecContext = ctx->streams[0]->codec;
        self.audioCodecContext = ctx->streams[1]->codec;
        self.ctx = ctx;
    }
    return self;
}

-(void)dealloc {
    if (self.audioDevice) {
        self.audioDevice->RegisterAudioCallback(NULL);
        self.audioDevice->Terminate();
        delete self.audioDevice;
    }
    delete self.audioTransport;
    
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);
    pthread_mutex_destroy(&_queueMutex);
    pthread_cond_destroy(&_queueCond);
    
    if (self.ctx) {
        closeRTMP(self.ctx);
    }
}

-(void)start {
    NSLog(@"start broadcaster");
    self.beginTS = [[NSDate date] timeIntervalSince1970];
    [self startCapture];
    [self startRecord];

    self.running = YES;
    self.encoding = YES;
    self.sending = YES;
    [self.packetQueue removeAllObjects];
    
    pthread_create(&_tid, NULL, encode_thread, (__bridge void*)self);
    pthread_create(&_sendTid, NULL, send_thread, (__bridge void*)self);
}

-(void)stop {
    [self stopCapture];
    [self stopRecord];
    
    self.running = NO;
    
    pthread_mutex_lock(&_mutex);
    self.encoding = NO;
    pthread_cond_signal(&_cond);
    pthread_mutex_unlock(&_mutex);

    pthread_mutex_lock(&_queueMutex);
    self.sending = NO;
    pthread_cond_signal(&_queueCond);
    pthread_mutex_unlock(&_queueMutex);
    
    pthread_join(_tid, NULL);
    pthread_join(_sendTid, NULL);
}

-(void)startCapture {
    //默认使用前置摄像头
    BOOL front = YES;
    AVCaptureDevice *device = nil;
    for (AVCaptureDevice *captureDevice in [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo] ) {
        if (front && captureDevice.position == AVCaptureDevicePositionFront) {
            device = captureDevice;
            break;
        } else if (!front && captureDevice.position == AVCaptureDevicePositionBack) {
            device = captureDevice;
            break;
        }
    }
    NSAssert(device, @"");
    NSLog(@"device:%@ %@", device.uniqueID, device.localizedName);
    [self.videoDevice setCaptureDeviceByUniqueId:device.uniqueID];
    
    struct VideoCaptureCapability cap;
    bzero(&cap, sizeof(cap));
    cap.width = CAPTURE_WIDTH;
    cap.height = CAPTURE_HEIGHT;
    cap.maxFPS = FPS;
    
    [self.videoDevice startCaptureWithCapability:cap];
}

-(void)stopCapture {
    [self.videoDevice stopCapture];
    //等待video队列中的数据被清空
    NSLog(@"waiting capture queue cleared...");
    dispatch_sync(self.videoDevice.queue, ^{
        NSLog(@"capture queue cleared");
    });
}


-(void)stopRecord {
    self.audioDevice->StopRecording();
}

-(void)startRecord {
    self.audioDevice->StartRecording();
}

-(void)onRecordData:(const void*)samples count:(int)count perSample:(int)bytePerSample
           channels:(int)channels samplesPerSecond:(int)samplesPerSecond {
    
    AVFrame *frame;
    int ret;
    
    /* frame containing input raw audio */
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }
    
    frame->nb_samples     = count;
    frame->format         = AV_SAMPLE_FMT_FLTP;
    frame->channel_layout = AV_CH_LAYOUT_MONO;
    frame->sample_rate = samplesPerSecond;
    

    double now = [[NSDate date] timeIntervalSince1970];
    frame->pts = (now - self.beginTS)*1000;
    
    /* setup the data pointers in the AVFrame */
    int size = count*bytePerSample;

    ret = avcodec_fill_audio_frame(frame, 1, AV_SAMPLE_FMT_FLTP,
                                   (const uint8_t*)samples, size, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not setup audio frame\n");
        exit(1);
    }
    
    
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL; // packet data will be allocated by the encoder
    pkt.size = 0;
    
    BOOL r = [self encodeAudio:frame packet:&pkt];
    if (r) {
        //        NSLog(@"record audio data:%d, %d, %d", count, bytePerSample, pkt.size);
        [self sendAudioPacket:&pkt];
        av_free_packet(&pkt);
    } else {
        NSLog(@"audio encode fail");
    }
}

static AVFrame* nv12_to_i420(SwsContext **sws, AVFrame *nv12frame, int dst_width, int dst_height) {
    AVFrame *frame;
    int stride;
    int ret;
    
    stride = nv12frame->width;
    
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = AV_PIX_FMT_YUV420P;
    frame->width  = dst_width;
    frame->height = dst_height;
    
    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize, frame->width, frame->height,
                         AV_PIX_FMT_YUV420P, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        exit(1);
    }
    
    struct SwsContext *sws_ctx =
    sws_getCachedContext(*sws, nv12frame->width, nv12frame->height, PIX_FMT_NV12, frame->width, frame->height, PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);
    
    sws_scale(sws_ctx, nv12frame->data, nv12frame->linesize, 0, nv12frame->height, frame->data, frame->linesize);
    
    *sws = sws_ctx;
    
    frame->pts = nv12frame->pts;

    return frame;
}

-(int32_t)IncomingFrame:(uint8_t*)videoFrame length:(int32_t)videoFrameLength
             capability:(struct VideoCaptureCapability)frameInfo time:(int64_t)captureTime {

    int ystride;
    int uvstride;
    AVFrame *frame;
    AVPacket pkt;
    
    ystride = frameInfo.width;
    uvstride = frameInfo.width;
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = PIX_FMT_NV12;
    frame->width  = frameInfo.width;
    frame->height = frameInfo.height;
    
    frame->linesize[0] = ystride;
    frame->linesize[1] = uvstride;
    
    frame->data[0] = videoFrame;
    frame->data[1] = videoFrame + ystride*frameInfo.height;
    
    double now = [[NSDate date] timeIntervalSince1970];
    frame->pts = (now - self.beginTS)*1000;
    
    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    
    AVFrame *i420_frame = nv12_to_i420(&_sws, frame, WIDTH, HEIGHT);

    pthread_mutex_lock(&_mutex);
    
    if (_captureFrame) {
        NSLog(@"overwrite capture frame");
        av_freep(&_captureFrame->data[0]);
        av_frame_free(&_captureFrame);
    }
    
    _captureFrame = i420_frame;
    pthread_cond_signal(&_cond);
    pthread_mutex_unlock(&_mutex);
    
    av_frame_free(&frame);
    return 0;
}

-(void)runEncoding {
    AVPacket pkt;
    AVFrame *frame = NULL;
    BOOL r;
    
    while (self.encoding) {
        BOOL blocked = NO;
        pthread_mutex_lock(&_queueMutex);
        
        if (self.packetQueue.count > 500) {
            blocked = YES;
        }
        pthread_mutex_unlock(&_queueMutex);
        
        if (blocked) {
            [NSThread sleepForTimeInterval:0.1];
            continue;
        }
        
        pthread_mutex_lock(&_mutex);
        
        while (_captureFrame == NULL && self.encoding) {
            pthread_cond_wait(&_cond, &_mutex);
        }
        if (!self.encoding) {
            pthread_mutex_unlock(&_mutex);
            break;
        }
        
        frame = _captureFrame;
        _captureFrame = NULL;
        pthread_mutex_unlock(&_mutex);
        
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;
        
        r = [self encode:frame packet:&pkt];
        
        if (r) {
            [self sendVideoPacket:&pkt];
            av_free_packet(&pkt);
        }
        
        av_freep(&frame->data[0]);
        av_frame_free(&frame);
    }
}


-(void)runSendLoop {
    double lastConnectTS = 0;
    AVFormatContext *ctx = self.ctx;
    
    while (self.sending) {
        pthread_mutex_lock(&_queueMutex);
        while (self.packetQueue.count == 0 && self.sending) {
            pthread_cond_wait(&_queueCond, &_queueMutex);
        }
        
        if (!self.sending) {
            pthread_mutex_unlock(&_queueMutex);
            break;
        }
        
        Packet *p = [self.packetQueue objectAtIndex:0];
        [self.packetQueue removeObjectAtIndex:0];
        
        NSLog(@"packet queue count:%zd", self.packetQueue.count);
        
        pthread_mutex_unlock(&_queueMutex);

        if (ctx->pb == NULL) {
            double now = [[NSDate date] timeIntervalSince1970];
            if (now - lastConnectTS < 1) {
                [NSThread sleepForTimeInterval:0.1];
                continue;
            }
            
            if (avio_open2(&ctx->pb, filename, AVIO_FLAG_WRITE, &_icb, NULL) < 0) {
                NSLog(@"connect rtmp server error");
                continue;
            }
            
            //video extra data
            if (avformat_write_header(ctx, NULL) < 0) {
                avio_close(ctx->pb);
                ctx->pb = NULL;
                NSLog(@"write rtmp header error");
                continue;
            }
            lastConnectTS = now;
        }
        
        int ret = av_write_frame(self.ctx, p.packet);
        
        if (ret < 0) {
            NSLog(@"write frame err:%d", ret);
            if (ctx->pb->error) {
                NSLog(@"close rtmp socket");
                avio_close(ctx->pb);
                ctx->pb = NULL;
            }
        }
    }
    
    if (ctx->pb != NULL) {
        avio_close(ctx->pb);
        ctx->pb = NULL;
    }
}

-(BOOL)encodeAudio:(struct AVFrame*)frame packet:(struct AVPacket*)packet {
    int ret, got_output;
    /* encode the samples */
    ret = avcodec_encode_audio2(self.audioCodecContext, packet, frame, &got_output);
    if (ret < 0) {
        fprintf(stderr, "Error encoding audio frame\n");
        return NO;
    }
    
    return got_output;
}



-(BOOL)encode:(AVFrame*)frame packet:(AVPacket*)packet {
    
    AVCodecContext *c= NULL;
    int ret, got_output;
    
    c = self.videoCodecContext;
    
    ret = avcodec_encode_video2(c, packet, frame, &got_output);
    if (ret < 0) {
        fprintf(stderr, "Error encoding frame\n");
        exit(1);
    }
    
    return got_output;
}

-(void)sendVideoPacket:(struct AVPacket*)packet {
    Packet *pkt = [[Packet alloc] init];
    
    av_copy_packet(pkt.packet, packet);
    pkt.audio = NO;
    pkt.packet->stream_index = 0;
    
    pthread_mutex_lock(&_queueMutex);
    
    [self.packetQueue addObject:pkt];
    
    pthread_cond_signal(&_queueCond);
    pthread_mutex_unlock(&_queueMutex);
    
}

-(void)sendAudioPacket:(struct AVPacket*)packet {
    Packet *pkt = [[Packet alloc] init];
    
    av_copy_packet(pkt.packet, packet);
    pkt.audio = YES;
    pkt.packet->stream_index = 1;

    pthread_mutex_lock(&_queueMutex);
    
    [self.packetQueue addObject:pkt];
    
    pthread_cond_signal(&_queueCond);
    pthread_mutex_unlock(&_queueMutex);
    
}


@end
