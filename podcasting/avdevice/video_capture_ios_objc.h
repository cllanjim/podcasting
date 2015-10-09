/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_IOS_VIDEO_CAPTURE_IOS_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_IOS_VIDEO_CAPTURE_IOS_OBJC_H_

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import "Video.h"

@protocol VideoCaptureDelegate <NSObject>

-(int32_t)IncomingFrame:(uint8_t*)videoFrame length:(int32_t)videoFrameLength
             capability:(struct VideoCaptureCapability)frameInfo time:(int64_t)captureTime;

@end


@interface VideoCaptureIosObjC
    : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate> {
 @private
  struct VideoCaptureCapability _capability;
  AVCaptureSession* _captureSession;
  int _captureId;
}
@property(nonatomic, weak) id<VideoCaptureDelegate> delegate;
@property(nonatomic) dispatch_queue_t queue;

@property enum VideoCaptureRotation frameRotation;

// custom initializer. Instance of VideoCaptureIos is needed
// for callback purposes.
// default init methods have been overridden to return nil.
- (id)initWithCaptureID:(int)captureId;
- (BOOL)setCaptureDeviceByUniqueId:(NSString*)uniequeId;
- (BOOL)startCaptureWithCapability:
    (const struct VideoCaptureCapability)capability;
- (BOOL)stopCapture;

@end
#endif  // WEBRTC_MODULES_VIDEO_CAPTURE_IOS_VIDEO_CAPTURE_IOS_OBJC_H_
