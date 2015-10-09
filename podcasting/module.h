/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_INTERFACE_MODULE_H_
#define MODULES_INTERFACE_MODULE_H_

#include <assert.h>

#include "typedefs.h"

namespace webrtc {

class Module {
 public:
  // TODO(henrika): Remove this when chrome is updated.
  // DEPRICATED Change the unique identifier of this object.
  virtual int32_t ChangeUniqueId(const int32_t id) { return 0; }

  // Returns the number of milliseconds until the module want a worker
  // thread to call Process.
  virtual int32_t TimeUntilNextProcess() = 0;

  // Process any pending tasks such as timeouts.
  virtual int32_t Process() = 0;

 protected:
  virtual ~Module() {}
};

}  // namespace webrtc

#endif  // MODULES_INTERFACE_MODULE_H_
