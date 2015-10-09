/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_IPHONE_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_IPHONE_H

#include <AudioUnit/AudioUnit.h>

#include "critical_section_posix.h"
#include "audio_device_defines.h"
#include <pthread.h>

namespace webrtc {
class ThreadWrapper;

const uint32_t N_REC_SAMPLES_PER_SEC = 44100;

const uint32_t N_REC_CHANNELS = 1;  // default is mono recording
const uint32_t N_DEVICE_CHANNELS = 8;

const uint32_t ENGINE_REC_BUF_SIZE_IN_SAMPLES = 1024;

// Number of 10 ms recording blocks in recording buffer
const uint16_t N_REC_BUFFERS = 20;

class AudioDeviceIPhone {
public:
    AudioDeviceIPhone(const int32_t id);
    ~AudioDeviceIPhone();



    // Main initializaton and termination
    virtual int32_t Init();
    virtual int32_t Terminate();
    virtual bool Initialized() const;

    // Device enumeration
    virtual int16_t RecordingDevices();


    // Device selection
    virtual int32_t SetRecordingDevice(uint16_t index);


    // Audio transport initialization
    virtual int32_t RecordingIsAvailable(bool& available);
    virtual int32_t InitRecording();
    virtual bool RecordingIsInitialized() const;

    // Audio transport control
    virtual int32_t StartRecording();
    virtual int32_t StopRecording();
    virtual bool Recording() const;


    // Delay information and control
    virtual int32_t RecordingDelay(uint16_t& delayMS) const;

public:
    // Reset Audio Deivce (for mobile devices only)
    virtual int32_t ResetAudioDevice();

    // Full-duplex transportation of PCM audio
    int32_t RegisterAudioCallback(AudioTransport* audioCallback) {
        _ptrCbAudioTransport = audioCallback;
        return 0;
    }

private:
    int32_t Id() {
        return _id;
    }

    // Init and shutdown
    int32_t InitPlayOrRecord();
    int32_t ShutdownPlayOrRecord();

    void UpdateRecordingDelay();
    void UpdatePlayoutDelay();

    static OSStatus RecordProcess(void *inRefCon,
                                  AudioUnitRenderActionFlags *ioActionFlags,
                                  const AudioTimeStamp *timeStamp,
                                  UInt32 inBusNumber,
                                  UInt32 inNumberFrames,
                                  AudioBufferList *ioData);



    OSStatus RecordProcessImpl(AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp *timeStamp,
                               uint32_t inBusNumber,
                               uint32_t inNumberFrames);



    static void* RunCapture(void* ptrThis);
    bool CaptureWorkerThread();
    
    
    int32_t DeliverRecordedData(int8_t *data, int nsamples);

private:
    CriticalSectionPosix *_critSect;

    pthread_t _captureWorkerThread;
    volatile bool _alive;

    int32_t _id;

    AudioUnit _auVoiceProcessing;

private:
    bool _initialized;
    bool _isShutDown;
    bool _recording;
    bool _recIsInitialized;

    bool _recordingDeviceIsSpecified;


    // The sampling rate to use with Audio Device Buffer
    uint32_t _adbSampFreq;

    // Delay calculation
    uint32_t _recordingDelay;
    uint32_t _recordingDelayHWAndOS;
    uint32_t _recordingDelayMeasurementCounter;


    // Recording buffers
    Float32
        _recordingBuffer[N_REC_BUFFERS][ENGINE_REC_BUF_SIZE_IN_SAMPLES];
    uint32_t _recordingLength[N_REC_BUFFERS];
    uint32_t _recordingSeqNumber[N_REC_BUFFERS];
    uint32_t _recordingCurrentSeq;

    // Current total size all data in buffers, used for delay estimate
    uint32_t _recordingBufferTotalSize;
    
    AudioTransport*                 _ptrCbAudioTransport;
    
    uint8_t                   _recChannels;
    // selected recording channel (left/right/both)
//    AudioDeviceModule::ChannelType _recChannel;
    
    // 2 or 4 depending on mono or stereo
    uint8_t                   _recBytesPerSample;
};

}  // namespace webrtc

#endif  // MODULES_AUDIO_DEVICE_MAIN_SOURCE_MAC_AUDIO_DEVICE_IPHONE_H_
