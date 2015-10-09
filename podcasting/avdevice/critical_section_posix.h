/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_CRITICAL_SECTION_POSIX_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_CRITICAL_SECTION_POSIX_H_


#include <pthread.h>

namespace webrtc {

class CriticalSectionPosix {
 public:
  CriticalSectionPosix();

  virtual ~CriticalSectionPosix();

  virtual void Enter();
  virtual void Leave();

 private:
  pthread_mutex_t mutex_;

};


    class  CriticalSectionScoped {
    public:
        explicit CriticalSectionScoped(CriticalSectionPosix* critsec)
        : ptr_crit_sec_(critsec) {
            ptr_crit_sec_->Enter();
        }
        
        ~CriticalSectionScoped() { ptr_crit_sec_->Leave(); }
        
    private:
        CriticalSectionPosix* ptr_crit_sec_;
    };
    
}  // namespace webrtc

#endif  // WEBRTC_SYSTEM_WRAPPERS_SOURCE_CRITICAL_SECTION_POSIX_H_
