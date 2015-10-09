/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <AudioToolbox/AudioServices.h>  // AudioSession

#include "audio_device_ios.h"
#include "typedefs.h"

#include <pthread.h>

#define NOSAMPLE10MS 1024

namespace webrtc {
AudioDeviceIPhone::AudioDeviceIPhone(const int32_t id)
    :
    _captureWorkerThread(NULL),
    _id(id),
    _auVoiceProcessing(NULL),
    _initialized(false),
    _isShutDown(false),
    _recording(false),
    _recIsInitialized(false),
    _recordingDeviceIsSpecified(false),
    _adbSampFreq(0),
    _recordingDelay(0),
    _recordingDelayHWAndOS(0),
    _recordingDelayMeasurementCounter(9999),
    _recordingCurrentSeq(0),
    _recordingBufferTotalSize(0),
    _alive(true),
    _recChannels(1),
    _recBytesPerSample(4){
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);

        _critSect = new CriticalSectionPosix();
    memset(_recordingBuffer, 0, sizeof(_recordingBuffer));
    memset(_recordingLength, 0, sizeof(_recordingLength));
    memset(_recordingSeqNumber, 0, sizeof(_recordingSeqNumber));
}

AudioDeviceIPhone::~AudioDeviceIPhone() {


    Terminate();

    delete _critSect;
}


// ============================================================================
//                                     API
// ============================================================================


int32_t AudioDeviceIPhone::Init() {


    CriticalSectionScoped lock(_critSect);

    if (_initialized) {
        return 0;
    }

    _isShutDown = false;

    // Create and start capture thread
    if (_captureWorkerThread == NULL) {
        pthread_t tid = NULL;
        int r = pthread_create(&tid, NULL, RunCapture, this);
        if (r != 0) {
 
            return -1;
        }
        
        _captureWorkerThread = tid;

    } else {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                     _id, "Thread already created");
    }


    _initialized = true;

    return 0;
}

int32_t AudioDeviceIPhone::Terminate() {
    if (!_initialized) {
        return 0;
    }

    // Stop capture thread
    if (_captureWorkerThread != NULL) {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice,
                     _id, "Stopping CaptureWorkerThread");

        //todo stop capture work thread
        _alive = false;
        pthread_join(_captureWorkerThread, NULL);
        
        _captureWorkerThread = NULL;
    }

    // Shut down Audio Unit
    ShutdownPlayOrRecord();

    _isShutDown = true;
    _initialized = false;
    _recordingDeviceIsSpecified = false;
    return 0;
}

bool AudioDeviceIPhone::Initialized() const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return (_initialized);
}




int16_t AudioDeviceIPhone::RecordingDevices() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    return (int16_t)1;
}

int32_t AudioDeviceIPhone::SetRecordingDevice(uint16_t index) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetRecordingDevice(index=%u)", index);

    if (_recIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording already initialized");
        return -1;
    }

    if (index !=0) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  SetRecordingDevice invalid index");
        return -1;
    }

    _recordingDeviceIsSpecified = true;

    return 0;
}



int32_t AudioDeviceIPhone::RecordingIsAvailable(bool& available) {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    available = false;

    // Try to initialize the recording side
    int32_t res = InitRecording();

    // Cancel effect of initialization
    StopRecording();

    if (res != -1) {
        available = true;
    }

    return 0;
}
    
int32_t AudioDeviceIPhone::InitRecording() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    if (!_initialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Not initialized");
        return -1;
    }

    if (_recording) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Recording already started");
        return -1;
    }

    if (_recIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Recording already initialized");
        return 0;
    }

    if (!_recordingDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording device is not specified");
        return -1;
    }



    _recIsInitialized = true;


    // Audio init
    if (InitPlayOrRecord() == -1) {
        // todo: Handle error
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitPlayOrRecord() failed");
    }
    

    return 0;
}

bool AudioDeviceIPhone::RecordingIsInitialized() const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);
    return (_recIsInitialized);
}

int32_t AudioDeviceIPhone::StartRecording() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    if (!_recIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording not initialized");
        return -1;
    }

    if (_recording) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Recording already started");
        return 0;
    }

    // Reset recording buffer
    memset(_recordingBuffer, 0, sizeof(_recordingBuffer));
    memset(_recordingLength, 0, sizeof(_recordingLength));
    memset(_recordingSeqNumber, 0, sizeof(_recordingSeqNumber));
    _recordingCurrentSeq = 0;
    _recordingBufferTotalSize = 0;
    _recordingDelay = 0;
    _recordingDelayHWAndOS = 0;
    // Make sure first call to update delay function will update delay
    _recordingDelayMeasurementCounter = 9999;


    // Start Audio Unit
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  Starting Audio Unit");
    OSStatus result = AudioOutputUnitStart(_auVoiceProcessing);
    if (0 != result) {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  Error starting Audio Unit (result=%d)", result);
        return -1;
    }


    _recording = true;

    return 0;
}

int32_t AudioDeviceIPhone::StopRecording() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    if (!_recIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Recording is not initialized");
        return 0;
    }

    _recording = false;


    // Both playout and recording has stopped, shutdown the device
    ShutdownPlayOrRecord();


    _recIsInitialized = false;

    return 0;
}

bool AudioDeviceIPhone::Recording() const {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);
    return (_recording);
}


// ----------------------------------------------------------------------------
//  ResetAudioDevice
//
//  Disable playout and recording, signal to capture thread to shutdown,
//  and set enable states after shutdown to same as current.
//  In capture thread audio device will be shutdown, then started again.
// ----------------------------------------------------------------------------
int32_t AudioDeviceIPhone::ResetAudioDevice() {
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    if (!_recIsInitialized) {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Playout or recording not initialized, doing nothing");
        return 0;  // Nothing to reset
    }

    // Store the states we have before stopping to restart below
    bool initRec = _recIsInitialized;
    bool rec = _recording;

    int res(0);

    // Stop playout and recording
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  Stopping playout and recording");
    res += StopRecording();



    if (initRec)  res += InitRecording();
    if (rec)      res += StartRecording();

    if (0 != res) {
        // Logging is done in init/start/stop calls above
        return -1;
    }

    return 0;
}


int32_t AudioDeviceIPhone::RecordingDelay(uint16_t& delayMS) const {
    delayMS = _recordingDelay;
    return 0;
}



// ============================================================================
//                                 Private Methods
// ============================================================================

    int32_t AudioDeviceIPhone::InitPlayOrRecord() {
        WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id, "%s", __FUNCTION__);
        
        OSStatus result = -1;
        
        // Check if already initialized
        if (NULL != _auVoiceProcessing) {
            // We already have initialized before and created any of the audio unit,
            // check that all exist
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                         "  Already initialized");
            // todo: Call AudioUnitReset() here and empty all buffers?
            return 0;
        }
        
        // Create Voice Processing Audio Unit
        AudioComponentDescription desc;
        AudioComponent comp;
        
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        
        comp = AudioComponentFindNext(NULL, &desc);
        if (NULL == comp) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not find audio component for Audio Unit");
            return -1;
        }
        
        result = AudioComponentInstanceNew(comp, &_auVoiceProcessing);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not create Audio Unit instance (result=%d)",
                         result);
            return -1;
        }
        
        // Set preferred hardware sample rate to 44 kHz
        Float64 sampleRate(44100.0);
        result = AudioSessionSetProperty(
                                         kAudioSessionProperty_PreferredHardwareSampleRate,
                                         sizeof(sampleRate), &sampleRate);
        if (0 != result) {
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                         "Could not set preferred sample rate (result=%d)", result);
        }
        
        uint32_t voiceChat = kAudioSessionMode_VoiceChat;
        AudioSessionSetProperty(kAudioSessionProperty_Mode,
                                sizeof(voiceChat), &voiceChat);
        
        //////////////////////
        // Setup Voice Processing Audio Unit
        
        // Note: For Signal Processing AU element 0 is output bus, element 1 is
        //       input bus for global scope element is irrelevant (always use
        //       element 0)
        
        // Enable IO on both elements
        
        // todo: Below we just log and continue upon error. We might want
        //       to close AU and return error for some cases.
        // todo: Log info about setup.
        
        UInt32 enableIO = 1;
        result = AudioUnitSetProperty(_auVoiceProcessing,
                                      kAudioOutputUnitProperty_EnableIO,
                                      kAudioUnitScope_Input,
                                      1,  // input bus
                                      &enableIO,
                                      sizeof(enableIO));
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not enable IO on input (result=%d)", result);
        }
        
        result = AudioUnitSetProperty(_auVoiceProcessing,
                                      kAudioOutputUnitProperty_EnableIO,
                                      kAudioUnitScope_Output,
                                      0,   // output bus
                                      &enableIO,
                                      sizeof(enableIO));
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not enable IO on output (result=%d)", result);
        }
        
        // Disable AU buffer allocation for the recorder, we allocate our own
        UInt32 flag = 0;
        result = AudioUnitSetProperty(
                                      _auVoiceProcessing, kAudioUnitProperty_ShouldAllocateBuffer,
                                      kAudioUnitScope_Output,  1, &flag, sizeof(flag));
        if (0 != result) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  Could not disable AU buffer allocation (result=%d)",
                         result);
            // Should work anyway
        }
        
        // Set recording callback
        AURenderCallbackStruct auCbS;
        memset(&auCbS, 0, sizeof(auCbS));
        auCbS.inputProc = RecordProcess;
        auCbS.inputProcRefCon = this;
        result = AudioUnitSetProperty(_auVoiceProcessing,
                                      kAudioOutputUnitProperty_SetInputCallback,
                                      kAudioUnitScope_Global, 1,
                                      &auCbS, sizeof(auCbS));
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not set record callback for Audio Unit (result=%d)",
                         result);
        }
        
        
        // Get stream format for out/0
        AudioStreamBasicDescription playoutDesc;
        UInt32 size = sizeof(playoutDesc);
        result = AudioUnitGetProperty(_auVoiceProcessing,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Output, 0, &playoutDesc,
                                      &size);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not get stream format Audio Unit out/0 (result=%d)",
                         result);
        }
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Audio Unit playout opened in sampling rate %f",
                     playoutDesc.mSampleRate);
        
        playoutDesc.mSampleRate = sampleRate;
        
        // Store the sampling frequency to use towards the Audio Device Buffer
        // todo: Add 48 kHz (increase buffer sizes). Other fs?
        if ((playoutDesc.mSampleRate > 44090.0)
            && (playoutDesc.mSampleRate < 44110.0)) {
            _adbSampFreq = 44100;
        } else if ((playoutDesc.mSampleRate > 15990.0)
                   && (playoutDesc.mSampleRate < 16010.0)) {
            _adbSampFreq = 16000;
        } else if ((playoutDesc.mSampleRate > 7990.0)
                   && (playoutDesc.mSampleRate < 8010.0)) {
            _adbSampFreq = 8000;
        } else {
            _adbSampFreq = 0;
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Audio Unit out/0 opened in unknown sampling rate (%f)",
                         playoutDesc.mSampleRate);
            // todo: We should bail out here.
        }
        
        
        // Set stream format for in/0  (use same sampling frequency as for out/0)
        playoutDesc.mFormatFlags = kLinearPCMFormatFlagIsFloat
        | kLinearPCMFormatFlagIsPacked
        | kLinearPCMFormatFlagIsNonInterleaved;
        playoutDesc.mBytesPerPacket = 4;
        playoutDesc.mFramesPerPacket = 1;
        playoutDesc.mBytesPerFrame = 4;
        playoutDesc.mChannelsPerFrame = 1;
        playoutDesc.mBitsPerChannel = 32;
        result = AudioUnitSetProperty(_auVoiceProcessing,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Input, 0, &playoutDesc, size);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not set stream format Audio Unit in/0 (result=%d)",
                         result);
        }
        
        // Get stream format for in/1
        AudioStreamBasicDescription recordingDesc;
        size = sizeof(recordingDesc);
        result = AudioUnitGetProperty(_auVoiceProcessing,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Input, 1, &recordingDesc,
                                      &size);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not get stream format Audio Unit in/1 (result=%d)",
                         result);
        }
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "  Audio Unit recording opened in sampling rate %f",
                     recordingDesc.mSampleRate);
        
        recordingDesc.mSampleRate = sampleRate;
        
        // Set stream format for out/1 (use same sampling frequency as for in/1)
        recordingDesc.mFormatFlags = kLinearPCMFormatFlagIsFloat
        | kLinearPCMFormatFlagIsPacked
        | kLinearPCMFormatFlagIsNonInterleaved;
        
        recordingDesc.mBytesPerPacket = 4;
        recordingDesc.mFramesPerPacket = 1;
        recordingDesc.mBytesPerFrame = 4;
        recordingDesc.mChannelsPerFrame = 1;
        recordingDesc.mBitsPerChannel = 32;
        result = AudioUnitSetProperty(_auVoiceProcessing,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Output, 1, &recordingDesc,
                                      size);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not set stream format Audio Unit out/1 (result=%d)",
                         result);
        }
        
        // Initialize here already to be able to get/set stream properties.
        result = AudioUnitInitialize(_auVoiceProcessing);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Could not init Audio Unit (result=%d)", result);
        }
        
        // Get hardware sample rate for logging (see if we get what we asked for)
        Float64 hardwareSampleRate = 0.0;
        size = sizeof(hardwareSampleRate);
        result = AudioSessionGetProperty(
                                         kAudioSessionProperty_CurrentHardwareSampleRate, &size,
                                         &hardwareSampleRate);
        if (0 != result) {
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  Could not get current HW sample rate (result=%d)", result);
        }
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  Current HW sample rate is %f, ADB sample rate is %d",
                     hardwareSampleRate, _adbSampFreq);
        
        return 0;
    }


int32_t AudioDeviceIPhone::ShutdownPlayOrRecord() {
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    // Close and delete AU
    OSStatus result = -1;
    if (NULL != _auVoiceProcessing) {
        result = AudioOutputUnitStop(_auVoiceProcessing);
        if (0 != result) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "  Error stopping Audio Unit (result=%d)", result);
        }
        result = AudioComponentInstanceDispose(_auVoiceProcessing);
        if (0 != result) {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "  Error disposing Audio Unit (result=%d)", result);
        }
        _auVoiceProcessing = NULL;
    }

    return 0;
}

// ============================================================================
//                                  Thread Methods
// ============================================================================

OSStatus
    AudioDeviceIPhone::RecordProcess(void *inRefCon,
                                     AudioUnitRenderActionFlags *ioActionFlags,
                                     const AudioTimeStamp *inTimeStamp,
                                     UInt32 inBusNumber,
                                     UInt32 inNumberFrames,
                                     AudioBufferList *ioData) {
    AudioDeviceIPhone* ptrThis = static_cast<AudioDeviceIPhone*>(inRefCon);

    return ptrThis->RecordProcessImpl(ioActionFlags,
                                      inTimeStamp,
                                      inBusNumber,
                                      inNumberFrames);
}


OSStatus
    AudioDeviceIPhone::RecordProcessImpl(
                                    AudioUnitRenderActionFlags *ioActionFlags,
                                    const AudioTimeStamp *inTimeStamp,
                                    uint32_t inBusNumber,
                                    uint32_t inNumberFrames) {
    // Setup some basic stuff
    // Use temp buffer not to lock up recording buffer more than necessary
    // todo: Make dataTmp a member variable with static size that holds
    //       max possible frames?
    Float32* dataTmp = new Float32[inNumberFrames];
    memset(dataTmp, 0, 4*inNumberFrames);

    AudioBufferList abList;
    abList.mNumberBuffers = 1;
    abList.mBuffers[0].mData = dataTmp;
    abList.mBuffers[0].mDataByteSize = 4*inNumberFrames;  // 4 bytes/sample
    abList.mBuffers[0].mNumberChannels = 1;

    // Get data from mic
    OSStatus res = AudioUnitRender(_auVoiceProcessing,
                                   ioActionFlags, inTimeStamp,
                                   inBusNumber, inNumberFrames, &abList);
    if (res != 0) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  Error getting rec data, error = %d", res);



        delete [] dataTmp;
        return 0;
    }


        
    if (_recording) {
        // Insert all data in temp buffer into recording buffers
        // There is zero or one buffer partially full at any given time,
        // all others are full or empty
        // Full means filled with noSamp10ms samples.

        CriticalSectionScoped lock(_critSect);
        
        const unsigned int noSamp10ms = NOSAMPLE10MS;
        unsigned int dataPos = 0;
        uint16_t bufPos = 0;
        int16_t insertPos = -1;
        unsigned int nCopy = 0;  // Number of samples to copy

        while (dataPos < inNumberFrames) {
            // Loop over all recording buffers or
            // until we find the partially full buffer
            // First choice is to insert into partially full buffer,
            // second choice is to insert into empty buffer
            bufPos = 0;
            insertPos = -1;
            nCopy = 0;
            while (bufPos < N_REC_BUFFERS) {
                if ((_recordingLength[bufPos] > 0)
                    && (_recordingLength[bufPos] < noSamp10ms)) {
                    // Found the partially full buffer
                    insertPos = static_cast<int16_t>(bufPos);
                    // Don't need to search more, quit loop
                    bufPos = N_REC_BUFFERS;
                } else if ((-1 == insertPos)
                           && (0 == _recordingLength[bufPos])) {
                    // Found an empty buffer
                    insertPos = static_cast<int16_t>(bufPos);
                }
                ++bufPos;
            }

            // Insert data into buffer
            if (insertPos > -1) {
                // We found a non-full buffer, copy data to it
                unsigned int dataToCopy = inNumberFrames - dataPos;
                unsigned int currentRecLen = _recordingLength[insertPos];
                unsigned int roomInBuffer = noSamp10ms - currentRecLen;
                nCopy = (dataToCopy < roomInBuffer ? dataToCopy : roomInBuffer);

                memcpy(&_recordingBuffer[insertPos][currentRecLen],
                       &dataTmp[dataPos], nCopy*sizeof(Float32));
                if (0 == currentRecLen) {
                    _recordingSeqNumber[insertPos] = _recordingCurrentSeq;
                    ++_recordingCurrentSeq;
                }
                _recordingBufferTotalSize += nCopy;
                // Has to be done last to avoid interrupt problems
                // between threads
                _recordingLength[insertPos] += nCopy;
                dataPos += nCopy;
            } else {
                // Didn't find a non-full buffer
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                             "  Could not insert into recording buffer");

                dataPos = inNumberFrames;  // Don't try to insert more
            }
        }
    }

    delete [] dataTmp;

    return 0;
}


void AudioDeviceIPhone::UpdateRecordingDelay() {
    ++_recordingDelayMeasurementCounter;

    if (_recordingDelayMeasurementCounter >= 100) {
        // Update HW and OS delay every second, unlikely to change

        _recordingDelayHWAndOS = 0;

        // HW input latency
        Float32 f32(0);
        UInt32 size = sizeof(f32);
        OSStatus result = AudioSessionGetProperty(
            kAudioSessionProperty_CurrentHardwareInputLatency, &size, &f32);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "error HW latency (result=%d)", result);
        }
        _recordingDelayHWAndOS += static_cast<int>(f32 * 1000000);

        // HW buffer duration
        f32 = 0;
        result = AudioSessionGetProperty(
            kAudioSessionProperty_CurrentHardwareIOBufferDuration, &size, &f32);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "error HW buffer duration (result=%d)", result);
        }
        _recordingDelayHWAndOS += static_cast<int>(f32 * 1000000);

        // AU latency
        Float64 f64(0);
        size = sizeof(f64);
        result = AudioUnitGetProperty(_auVoiceProcessing,
                                      kAudioUnitProperty_Latency,
                                      kAudioUnitScope_Global, 0, &f64, &size);
        if (0 != result) {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "error AU latency (result=%d)", result);
        }
        _recordingDelayHWAndOS += static_cast<int>(f64 * 1000000);

        // To ms
        _recordingDelayHWAndOS = (_recordingDelayHWAndOS - 500) / 1000;

        // Reset counter
        _recordingDelayMeasurementCounter = 0;
    }

    _recordingDelay = _recordingDelayHWAndOS;

    // ADB recording buffer size, update every time
    // Don't count the one next 10 ms to be sent, then convert samples => ms
    const uint32_t noSamp10ms = NOSAMPLE10MS;
    if (_recordingBufferTotalSize > noSamp10ms) {
        _recordingDelay +=
            (_recordingBufferTotalSize - noSamp10ms) / (_adbSampFreq / 1000);
    }
}

void* AudioDeviceIPhone::RunCapture(void* ptrThis) {
    AudioDeviceIPhone *t = (AudioDeviceIPhone*)ptrThis;
    while (t->_alive) {
        static_cast<AudioDeviceIPhone*>(ptrThis)->CaptureWorkerThread();
    }
    return 0;
}

int32_t AudioDeviceIPhone::DeliverRecordedData(int8_t *data, int nsamples) {
        // Ensure that user has initialized all essential members
        if ((_adbSampFreq == 0)     ||
            (nsamples == 0)        ||
            (_recBytesPerSample == 0) ||
            (_recChannels == 0))
        {
            assert(false);
            return -1;
        }
        
        if (_ptrCbAudioTransport == NULL)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "failed to deliver recorded data (AudioTransport does not exist)");
            return 0;
        }
        
        int32_t res(0);
        uint32_t newMicLevel(0);
        uint32_t totalDelayMS = 0;
        
        res = _ptrCbAudioTransport->RecordedDataIsAvailable(data,
                                                            nsamples,
                                                            _recBytesPerSample,
                                                            _recChannels,
                                                            _adbSampFreq,
                                                            totalDelayMS,
                                                            0,
                                                            0,
                                                            0,
                                                            newMicLevel);
        if (res != -1)
        {

        }
        
        return 0;
    }

    

bool AudioDeviceIPhone::CaptureWorkerThread() {
    Float32 audio[ENGINE_REC_BUF_SIZE_IN_SAMPLES];
    int audioLen = 0;
    if (_recording) {
        CriticalSectionScoped lock(_critSect);
        int bufPos = 0;
        unsigned int lowestSeq = 0;
        int lowestSeqBufPos = 0;
        bool foundBuf = true;
        const unsigned int noSamp10ms = NOSAMPLE10MS;

        while (foundBuf) {
            // Check if we have any buffer with data to insert
            // into the Audio Device Buffer,
            // and find the one with the lowest seq number
            foundBuf = false;
            for (bufPos = 0; bufPos < N_REC_BUFFERS; ++bufPos) {
                if (noSamp10ms == _recordingLength[bufPos]) {
                    if (!foundBuf) {
                        lowestSeq = _recordingSeqNumber[bufPos];
                        lowestSeqBufPos = bufPos;
                        foundBuf = true;
                    } else if (_recordingSeqNumber[bufPos] < lowestSeq) {
                        lowestSeq = _recordingSeqNumber[bufPos];
                        lowestSeqBufPos = bufPos;
                    }
                }
            }  // for

            // Insert data into the Audio Device Buffer if found any
            if (foundBuf) {
                // Update recording delay
                UpdateRecordingDelay();

                memcpy(audio, reinterpret_cast<int8_t*>(_recordingBuffer[lowestSeqBufPos]), _recordingLength[lowestSeqBufPos]*_recBytesPerSample);
                audioLen = _recordingLength[lowestSeqBufPos];
              
                // Make buffer available
                _recordingSeqNumber[lowestSeqBufPos] = 0;
                _recordingBufferTotalSize -= _recordingLength[lowestSeqBufPos];
                // Must be done last to avoid interrupt problems between threads
                _recordingLength[lowestSeqBufPos] = 0;
            }
        }  // while (foundBuf)
    }  // if (_recording)

    if (audioLen > 0) {
        DeliverRecordedData((int8_t*)audio, audioLen);
    }
    
    {
        // Normal case
        // Sleep thread (5ms) to let other threads get to work
        // todo: Is 5 ms optimal? Sleep shorter if inserted into the Audio
        //       Device Buffer?
        timespec t;
        t.tv_sec = 0;
        t.tv_nsec = 5*1000*1000;
        nanosleep(&t, NULL);
    }

    return true;
}

}  // namespace webrtc
