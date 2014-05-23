/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CORE_MEDIA_BUFFER_GROUP_H_
#define CORE_MEDIA_BUFFER_GROUP_H_

#include "CoreMediaBuffer.h"
#include <utils/Errors.h>
#include <utils/threads.h>

namespace android {

class CoreMediaBuffer;
class MetaData;

class CoreMediaBufferGroup : public CoreMediaBufferObserver {
public:
    CoreMediaBufferGroup();
    ~CoreMediaBufferGroup();

    void add_buffer(CoreMediaBuffer *buffer);

    // Blocks until a buffer is available and returns it to the caller,
    // the returned buffer will have a reference count of 1.
    status_t acquire_buffer(CoreMediaBuffer **buffer);

protected:
    virtual void signalBufferReturned(CoreMediaBuffer *buffer);

private:
    friend class CoreMediaBuffer;

    Mutex mLock;
    Condition mCondition;

    CoreMediaBuffer *mFirstBuffer, *mLastBuffer;

    CoreMediaBufferGroup(const CoreMediaBufferGroup &);
    CoreMediaBufferGroup &operator=(const CoreMediaBufferGroup &);
};

}  // namespace android

#endif  // CORE_MEDIA_BUFFER_GROUP_H_
