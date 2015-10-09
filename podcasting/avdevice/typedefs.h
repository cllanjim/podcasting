/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This file contains platform-specific typedefs and defines.
// Much of it is derived from Chromium's build/build_config.h.

#ifndef WEBRTC_TYPEDEFS_H_
#define WEBRTC_TYPEDEFS_H_


#define WEBRTC_MAC
#define WEBRTC_IOS
#define WEBRTC_CLOCK_TYPE_REALTIME

// For access to standard POSIXish features, use WEBRTC_POSIX instead of a
// more specific macro.
#if defined(WEBRTC_MAC) || defined(WEBRTC_LINUX) || \
    defined(WEBRTC_ANDROID)
#define WEBRTC_POSIX
#endif

// Processor architecture detection.  For more info on what's defined, see:
//   http://msdn.microsoft.com/en-us/library/b0084kay.aspx
//   http://www.agner.org/optimize/calling_conventions.pdf
//   or with gcc, run: "echo | gcc -E -dM -"
//#if defined(_M_X64) || defined(__x86_64__)
//#define WEBRTC_ARCH_X86_FAMILY
//#define WEBRTC_ARCH_X86_64
//#define WEBRTC_ARCH_64_BITS
//#define WEBRTC_ARCH_LITTLE_ENDIAN
//#elif defined(_M_IX86) || defined(__i386__)
//#define WEBRTC_ARCH_X86_FAMILY
//#define WEBRTC_ARCH_X86
//#define WEBRTC_ARCH_32_BITS
//#define WEBRTC_ARCH_LITTLE_ENDIAN
//#elif defined(__ARMEL__)
//// TODO(ajm): We'd prefer to control platform defines here, but this is
//// currently provided by the Android makefiles. Commented to avoid duplicate
//// definition warnings.
////#define WEBRTC_ARCH_ARM
//// TODO(ajm): Chromium uses the following two defines. Should we switch?
////#define WEBRTC_ARCH_ARM_FAMILY
////#define WEBRTC_ARCH_ARMEL
//#define WEBRTC_ARCH_32_BITS
//#define WEBRTC_ARCH_LITTLE_ENDIAN
//#elif defined(__MIPSEL__)
//#define WEBRTC_ARCH_32_BITS
//#define WEBRTC_ARCH_LITTLE_ENDIAN
//#else
//#error Please add support for your architecture in typedefs.h
//#endif
//
//#if !(defined(WEBRTC_ARCH_LITTLE_ENDIAN) ^ defined(WEBRTC_ARCH_BIG_ENDIAN))
//#error Define either WEBRTC_ARCH_LITTLE_ENDIAN or WEBRTC_ARCH_BIG_ENDIAN
//#endif

#if defined(__SSE2__) || defined(_MSC_VER)
#define WEBRTC_USE_SSE2
#endif

#if !defined(_MSC_VER)
#include <stdint.h>
#else
// Define C99 equivalent types, since MSVC doesn't provide stdint.h.
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef __int64             int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned __int64    uint64_t;
#endif

// Borrowed from Chromium's base/compiler_specific.h.
// Annotate a virtual method indicating it must be overriding a virtual
// method in the parent class.
// Use like:
//   virtual void foo() OVERRIDE;
#if defined(_MSC_VER)
#define OVERRIDE override
#elif defined(__clang__)
// Clang defaults to C++03 and warns about using override. Squelch that.
// Intentionally no push/pop here so all users of OVERRIDE ignore the warning
// too. This is like passing -Wno-c++11-extensions, except that GCC won't die
// (because it won't see this pragma).
#pragma clang diagnostic ignored "-Wc++11-extensions"
#define OVERRIDE override
#elif defined(__GNUC__) && __cplusplus >= 201103 && \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40700
// GCC 4.7 supports explicit virtual overrides when C++11 support is enabled.
#define OVERRIDE override
#else
#define OVERRIDE
#endif

// Annotate a function indicating the caller must examine the return value.
// Use like:
//   int foo() WARN_UNUSED_RESULT;
// TODO(ajm): Hack to avoid multiple definitions until the base/ of webrtc and
// libjingle are merged.
#if !defined(WARN_UNUSED_RESULT)
#if defined(__GNUC__)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif
#endif  // WARN_UNUSED_RESULT



enum TraceModule
{
    kTraceUndefined              = 0,
    // not a module, triggered from the engine code
    kTraceVoice                  = 0x0001,
    // not a module, triggered from the engine code
    kTraceVideo                  = 0x0002,
    // not a module, triggered from the utility code
    kTraceUtility                = 0x0003,
    kTraceRtpRtcp                = 0x0004,
    kTraceTransport              = 0x0005,
    kTraceSrtp                   = 0x0006,
    kTraceAudioCoding            = 0x0007,
    kTraceAudioMixerServer       = 0x0008,
    kTraceAudioMixerClient       = 0x0009,
    kTraceFile                   = 0x000a,
    kTraceAudioProcessing        = 0x000b,
    kTraceVideoCoding            = 0x0010,
    kTraceVideoMixer             = 0x0011,
    kTraceAudioDevice            = 0x0012,
    kTraceVideoRenderer          = 0x0014,
    kTraceVideoCapture           = 0x0015,
    kTraceVideoPreocessing       = 0x0016,
    kTraceRemoteBitrateEstimator = 0x0017,
};

enum TraceLevel
{
    kTraceNone               = 0x0000,    // no trace
    kTraceStateInfo          = 0x0001,
    kTraceWarning            = 0x0002,
    kTraceError              = 0x0004,
    kTraceCritical           = 0x0008,
    kTraceApiCall            = 0x0010,
    kTraceDefault            = 0x00ff,
    
    kTraceModuleCall         = 0x0020,
    kTraceMemory             = 0x0100,   // memory info
    kTraceTimer              = 0x0200,   // timing info
    kTraceStream             = 0x0400,   // "continuous" stream of data
    
    // used for debug purposes
    kTraceDebug              = 0x0800,  // debug
    kTraceInfo               = 0x1000,  // debug info
    
    // Non-verbose level used by LS_INFO of logging.h. Do not use directly.
    kTraceTerseInfo          = 0x2000,
    
    kTraceAll                = 0xffff
};


inline void TraceAdd(const TraceLevel level, const TraceModule module,
                     const int32_t id, const char* msg, ...) {}
#define WEBRTC_TRACE true ? (void) 0 : TraceAdd

#endif  // WEBRTC_TYPEDEFS_H_
